#include "core.h"
#include "prng.h"
#include "time.h"
#include <format>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ScreenBuffer {
    HBITMAP bitmap{};
    HDC buffer_dc{};
    s32 width{};
    s32 height{};
};

static volatile bool g_run_game{true};
static ScreenBuffer g_screen_buffer{};
static Prng<u16> g_color_rng = Prng<u16>::create(255);

static void screen_buffer_init(HWND window, ScreenBuffer& screen_buffer)
{
    // resolve window size
    RECT rect{};
    MUSTE(GetClientRect(window, &rect));
    auto width = rect.right - rect.left;
    auto height = rect.bottom - rect.top;
    DEBUG_PRINT(
        std::format("screen_buffer_init(): width={} height={}\n", width, height)
            .c_str()
    );

    // create DCs (device context) for off-screen drawing
    HDC window_dc = MUST(GetDC(window));
    HDC buffer_dc = MUST(CreateCompatibleDC(window_dc));

    // create bitmap and associate it with our buffer DC
    HBITMAP bitmap = MUST(CreateCompatibleBitmap(window_dc, width, height));
    auto result = SelectObject(buffer_dc, bitmap);
    if (result == nullptr || result == HGDI_ERROR) {
        PANICM("SelectObject() failed");
    }

    screen_buffer.bitmap = bitmap;
    screen_buffer.buffer_dc = buffer_dc;
    screen_buffer.width = width;
    screen_buffer.height = height;

    MUST(ReleaseDC(window, window_dc));
}

static void screen_buffer_release(ScreenBuffer& screen_buffer)
{
    DeleteDC(screen_buffer.buffer_dc);
    DeleteObject(screen_buffer.bitmap);
    ZeroMemory(&screen_buffer, sizeof(ScreenBuffer));
}

static void screen_buffer_fill_random(ScreenBuffer& screen_buffer)
{
    auto red = g_color_rng.next();
    auto green = g_color_rng.next();
    auto blue = g_color_rng.next();
    auto color = RGB(red, green, blue);

    for (s32 y = 0; y < screen_buffer.height; ++y) {
        for (s32 x = 0; x < screen_buffer.width; ++x) {
            SetPixel(screen_buffer.buffer_dc, x, y, color);
        }
    }
}

static void screen_buffer_blit(HWND window, ScreenBuffer& screen_buffer)
{
    DEBUG_PRINT("screen_buffer_blit()\n");

    // invalidate the whole window (client area) so it gets redrawn completely
    MUST(InvalidateRect(window, NULL, true));

    PAINTSTRUCT ps{};
    HDC window_dc = MUST(BeginPaint(window, &ps));

    MUSTE(BitBlt(
        window_dc,               // dest DC
        0,                       // dest upper-left X
        0,                       // dest upper-left Y
        screen_buffer.width,     // dest and src rect width
        screen_buffer.height,    // dest and src rect height
        screen_buffer.buffer_dc, // src
        0,                       // src upper-left X
        0,                       // src upper-left Y
        SRCCOPY
    ));

    EndPaint(window, &ps);
}

static void draw(HWND window, ScreenBuffer& screen_buffer)
{
    screen_buffer_fill_random(screen_buffer);
    screen_buffer_blit(window, screen_buffer);
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

    case WM_PAINT:
        DEBUG_PRINT("WM_PAINT\n");
        draw(window, g_screen_buffer);
        return 0;

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
        draw(window, g_screen_buffer);
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
