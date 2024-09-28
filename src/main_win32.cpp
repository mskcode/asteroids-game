#include "core.h"
#include "prng.h"
#include "time.h"
#include <format>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

union ARGB {
    u32 value;
    u8 data[4];

    // color components;
    // the order is important since the value is stored in little-endian
    // format: i.e. instead of RGB, it's stored as BGR; the alpha channel is
    // currently unused
    struct Colors {
        u8 blue;
        u8 green;
        u8 red;
        u8 alpha;
    } colors;
};

struct ScreenBuffer {
    BITMAPINFO bitmap_info;
    ARGB* pixels;
    s32 width;
    s32 height;
};

static volatile bool g_run_game{true};
static ScreenBuffer g_screen_buffer{};
static engine::prng::Prng<u16> g_color_rng = engine::prng::Prng<u16>::create(255
);

static void screen_buffer_init(HWND window, ScreenBuffer& screen_buffer)
{
    // resolve window size
    RECT rect{};
    MUSTE(GetClientRect(window, &rect));
    s32 width = rect.right - rect.left;
    s32 height = rect.bottom - rect.top;

    // configure screen buffer bounds
    screen_buffer.width = width;
    screen_buffer.height = height;

    // setup bitmap info
    screen_buffer.bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    screen_buffer.bitmap_info.bmiHeader.biWidth = width;    // width
    screen_buffer.bitmap_info.bmiHeader.biHeight = -height; // top-down bitmap
    screen_buffer.bitmap_info.bmiHeader.biPlanes = 1;       // must be 1
    screen_buffer.bitmap_info.bmiHeader.biBitCount = 32;    // 32-bit color
    screen_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB; // BI_BITFIELDS
    screen_buffer.bitmap_info.bmiHeader.biSizeImage = 0; // 0 when uncompressed

    // allocate pixel buffer
    size_t pixel_count = static_cast<size_t>(width * height);
    screen_buffer.pixels = new ARGB[pixel_count];
    ZeroMemory(screen_buffer.pixels, width * height * sizeof(ARGB));

    // create DCs (device context) for off-screen drawing
    HDC window_dc = MUST(GetDC(window));
    HDC buffer_dc = MUST(CreateCompatibleDC(window_dc));

    // create bitmap and associate it with our buffer DC
    HBITMAP bitmap = MUST(CreateCompatibleBitmap(window_dc, width, height));
    auto result = SelectObject(buffer_dc, bitmap);
    if (result == nullptr || result == HGDI_ERROR) {
        PANICM("SelectObject() failed");
    }
}

static void screen_buffer_release(ScreenBuffer& screen_buffer)
{
    delete[] screen_buffer.pixels;
    ZeroMemory(&screen_buffer, sizeof(ScreenBuffer));
}

static void
screen_buffer_draw_pixel(ScreenBuffer& screen_buffer, s32 x, s32 y, ARGB color)
{
    if (x < 0 || x >= screen_buffer.width || y < 0 ||
        y >= screen_buffer.height) {
        return;
    }

    screen_buffer.pixels[y * screen_buffer.width + x] = color;
}

static void screen_buffer_fill_random(ScreenBuffer& screen_buffer)
{
    ARGB argb{};
    //argb.colors.red = 0xFF;   // red
    //argb.colors.blue = 0x00;  // blue
    //argb.colors.green = 0x00; // green

    argb.colors.red = g_color_rng.nextAs<u8>();
    argb.colors.green = g_color_rng.nextAs<u8>();
    argb.colors.blue = g_color_rng.nextAs<u8>();

    for (s32 y = 0; y < screen_buffer.height; ++y) {
        for (s32 x = 0; x < screen_buffer.width; ++x) {
            screen_buffer_draw_pixel(screen_buffer, x, y, argb);
        }
    }
}

static void screen_buffer_blit(HDC device_context, ScreenBuffer& screen_buffer)
{
    DEBUG_PRINT("screen_buffer_blit2()\n");

    // invalidate the whole window (client area) so it gets redrawn completely
    // MUST(InvalidateRect(window, NULL, true));

    // prepare the bitmap
    HDC memory_dc = CreateCompatibleDC(device_context);
    HBITMAP bitmap = CreateCompatibleBitmap(
        device_context,
        screen_buffer.width,
        screen_buffer.height
    );
    SelectObject(memory_dc, bitmap);

    // transfer the pixel data to the bitmap
    SetDIBits(
        memory_dc, // target; device context
        bitmap,    // target; i.e. bitmap to be altered
        0,         // start scan line
        static_cast<u32>(screen_buffer.height), // number of scan lines
        screen_buffer.pixels,                   // source
        &screen_buffer.bitmap_info,             // bitmap info
        DIB_RGB_COLORS                          // literal RGB values
    );

    MUSTE(BitBlt(
        device_context,       // dest DC
        0,                    // dest upper-left X
        0,                    // dest upper-left Y
        screen_buffer.width,  // dest and src rect width
        screen_buffer.height, // dest and src rect height
        memory_dc,            // src
        0,                    // src upper-left X
        0,                    // src upper-left Y
        SRCCOPY
    ));

    // clean up
    DeleteObject(bitmap);
    DeleteDC(memory_dc);
}

static void draw(HDC device_context, ScreenBuffer& screen_buffer)
{
    screen_buffer_fill_random(screen_buffer);
    screen_buffer_blit(device_context, screen_buffer);
}

LRESULT CALLBACK window_procedure(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) noexcept
{
    switch (message) {
        case WM_CREATE:
            DEBUG_PRINT("WM_CREATE\n");
            screen_buffer_init(window, g_screen_buffer);
            return 0;

        case WM_SIZE:
            DEBUG_PRINT("WM_SIZE\n");
            screen_buffer_release(g_screen_buffer);
            screen_buffer_init(window, g_screen_buffer);
            return 0;

        case WM_PAINT: {
            DEBUG_PRINT("WM_PAINT\n");
            PAINTSTRUCT ps{};
            HDC window_dc = MUST(BeginPaint(window, &ps));
            draw(window_dc, g_screen_buffer);
            EndPaint(window, &ps);
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            DEBUG_PRINT("WM_CLOSE | WM_DESTROY\n");
            PostQuitMessage(0);
            return 0;
    }

    // Default behavior for other messages
    return DefWindowProc(window, message, wParam, lParam);
}

static void win32_message_pump()
{
    const u16 max_events_per_cycle = 20;
    for (u16 i = 0; i < max_events_per_cycle; ++i) {
        MSG message;
        bool had_message = PeekMessage(&message, 0, 0, 0, PM_REMOVE);
        if (!had_message) {
            // no messages; break out of the loop
            break;
        }

        // handle special events that never reach the window procedure
        switch (message.message) {
            case WM_QUIT:
                g_run_game = false;
                break;
        }

        // handle the message
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

int APIENTRY _tWinMain(
    HINSTANCE instance,
    [[maybe_unused]] HINSTANCE prev_instance,
    [[maybe_unused]] LPWSTR cmd_line,
    int cmd_show
)
{
    const auto* windowTitle = L"Minimal Win32 Window";
    const auto* className = L"MinimalWindow";

    // Define and register a window class
    WNDCLASS wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;            // Redraw on resize
    wc.lpfnWndProc = window_procedure;             // Set the window procedure
    wc.hInstance = instance;                       // Set the instance handle
    wc.lpszClassName = className;                  // Set the class name
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Set the background color

    MUST(RegisterClass(&wc));

    // Create the window
    HWND window = MUST(CreateWindowEx(
        0,                   // Optional window styles
        className,           // Window class
        windowTitle,         // Window title
        WS_OVERLAPPEDWINDOW, // Window style
        CW_USEDEFAULT,       // X position
        CW_USEDEFAULT,       // Y position
        800,                 // width
        600,                 // height
        NULL,                // Parent window
        NULL,                // Menu
        instance,            // Instance handle
        NULL                 // Additional application data
    ));

    ShowWindow(window, cmd_show);

    while (g_run_game) {
        auto stopwatch = engine::time::Stopwatch::start();
        win32_message_pump();

        HDC window_dc = MUST(GetDC(window));
        draw(window_dc, g_screen_buffer);
        ReleaseDC(window, window_dc);

        DEBUG_PRINT(
            std::format(
                "{} ms\n",
                stopwatch.split().value(engine::time::TimeUnit::MILLISECONDS)
            )
                .c_str()
        );

        // Sleep(500);
    }

    return 0;
}
