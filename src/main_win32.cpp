#include "core.h"
#include "prng.h"
#include "time.h"
#include <cstring>
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
    u64 pixels_size;
    s32 width;
    s32 height;
    u32 scanlines; // same as height
};

static volatile bool g_run_game{true};
static ScreenBuffer g_screen_buffer{};
static engine::prng::Prng<u16> g_color_rng = engine::prng::Prng<u16>::create(255
);
static engine::prng::Prng<u32>
    g_rng = engine::prng::Prng<u32>::create_with_natural_limits();

static auto argb_create_random() -> ARGB
{
    ARGB argb{};
    argb.colors.red = g_color_rng.next_as<u8>();
    argb.colors.green = g_color_rng.next_as<u8>();
    argb.colors.blue = g_color_rng.next_as<u8>();
    return argb;
}

static auto argb_create(u8 red, u8 green, u8 blue) -> ARGB
{
    ARGB argb{};
    argb.colors.red = red;
    argb.colors.green = green;
    argb.colors.blue = blue;
    return argb;
}

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
    screen_buffer.scanlines = static_cast<u32>(height);

    // setup bitmap info
    screen_buffer.bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    screen_buffer.bitmap_info.bmiHeader.biWidth = width;    // width
    screen_buffer.bitmap_info.bmiHeader.biHeight = -height; // top-down bitmap
    screen_buffer.bitmap_info.bmiHeader.biPlanes = 1;       // must be 1
    screen_buffer.bitmap_info.bmiHeader.biBitCount = 32;    // 32-bit color
    screen_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB; // BI_BITFIELDS
    screen_buffer.bitmap_info.bmiHeader.biSizeImage = 0; // 0 when uncompressed

    // allocate pixel buffer
    u64 pixel_size = static_cast<u64>(width * height);
    screen_buffer.pixels_size = pixel_size;
    screen_buffer.pixels = new ARGB[pixel_size];
    ZeroMemory(screen_buffer.pixels, width * height * sizeof(ARGB));
}

static void screen_buffer_release(ScreenBuffer& screen_buffer)
{
    delete[] screen_buffer.pixels;
    ZeroMemory(&screen_buffer, sizeof(ScreenBuffer));
}

static void
screen_buffer_draw_pixel(ScreenBuffer& screen_buffer, s32 x, s32 y, ARGB color)
{
    // FIXME: change to assert
    if (x < 0 || x >= screen_buffer.width || y < 0 ||
        y >= screen_buffer.height) {
        return;
    }

    auto index = y * screen_buffer.width + x;
    screen_buffer.pixels[index] = color;
}

static void screen_buffer_fill(ScreenBuffer& screen_buffer, ARGB color)
{
    for (u64 i = 0; i < screen_buffer.pixels_size; ++i) {
        screen_buffer.pixels[i] = color;
    }
}

static void screen_buffer_fill_random(ScreenBuffer& screen_buffer)
{
    ARGB argb{};
    argb.colors.red = g_color_rng.next_as<u8>();
    argb.colors.green = g_color_rng.next_as<u8>();
    argb.colors.blue = g_color_rng.next_as<u8>();
    screen_buffer_fill(screen_buffer, argb);
}

static void screen_buffer_blit(HDC device_context, ScreenBuffer& screen_buffer)
{
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
        memory_dc,                  // target; device context
        bitmap,                     // target; i.e. bitmap to be altered
        0,                          // start scan line
        screen_buffer.scanlines,    // number of scan lines
        screen_buffer.pixels,       // source
        &screen_buffer.bitmap_info, // bitmap info
        DIB_RGB_COLORS              // literal RGB values
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

//============================================================================
// Game
//============================================================================

struct Particle {
    s32 x{};
    s32 y{};
    ARGB color{};
    s32 velocity_x{};
    s32 velocity_y{};
    engine::time::Instant birth_time{};
};

static constexpr u32 MAX_PARTICLES = 100;
static Particle g_particles[MAX_PARTICLES];

static void particles_init(ScreenBuffer& screen_buffer)
{
    auto screen_width = static_cast<u32>(screen_buffer.width);
    auto screen_height = static_cast<u32>(screen_buffer.height);

    for (u32 i = 0; i < MAX_PARTICLES; ++i) {
        s32 x = static_cast<s32>(g_rng.next() % screen_width);
        s32 y = static_cast<s32>(g_rng.next() % screen_height);
        g_particles[i] = {
            x,                    // x
            y,                    // y
            argb_create_random(), // color
            0,                    // velocity_x
            0,                    // velocity_y
        };
    }
}

static void particles_draw(ScreenBuffer& screen_buffer)
{
    for (u32 i = 0; i < MAX_PARTICLES; ++i) {
        Particle& particle = g_particles[i];
        screen_buffer_draw_pixel(
            screen_buffer,
            particle.x,
            particle.y,
            particle.color
        );
    }
}

static void game_update([[maybe_unused]] engine::time::Duration delta)
{
    DEBUG_PRINT(std::format(
                    "previous UPDATE was {} ms ago\n",
                    delta.value(engine::time::TimeUnit::MILLISECONDS)
    )
                    .c_str());
}

static void game_render(
    [[maybe_unused]] engine::time::Duration delta,
    ScreenBuffer& screen_buffer
)
{
    DEBUG_PRINT(std::format(
                    "previous RENDER was {} ms ago\n",
                    delta.value(engine::time::TimeUnit::MILLISECONDS)
    )
                    .c_str());

    static ARGB black = argb_create(0x00, 0x00, 0x00);
    screen_buffer_fill(screen_buffer, black);
    particles_draw(screen_buffer);
}

//============================================================================
// Win32 windowing
//============================================================================

static LRESULT CALLBACK window_procedure(
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
            particles_init(g_screen_buffer);
            return 0;

        case WM_SIZE:
            DEBUG_PRINT("WM_SIZE\n");
            screen_buffer_release(g_screen_buffer);
            screen_buffer_init(window, g_screen_buffer);
            return 0;

        // this is a request to repaint the window that resides outside of the
        // normal game update-render-loop
        case WM_PAINT: {
            DEBUG_PRINT("WM_PAINT\n");
            PAINTSTRUCT ps{};
            HDC window_dc = MUST(BeginPaint(window, &ps));
            screen_buffer_blit(window_dc, g_screen_buffer);
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

    engine::time::TickLimiter update_tick_limiter{30};
    engine::time::TickLimiter render_tick_limiter{60};

    while (g_run_game) {
        auto stopwatch = engine::time::Stopwatch::start();

        // handle mandatory window messages
        win32_message_pump();

        if (update_tick_limiter.should_tick()) {
            game_update(update_tick_limiter.time_from_last_tick());
            update_tick_limiter.tick();
        }

        if (render_tick_limiter.should_tick()) {
            game_render(
                render_tick_limiter.time_from_last_tick(),
                g_screen_buffer
            );
            render_tick_limiter.tick();
        }

        // render the contents of the screen buffer into window
        HDC window_dc = MUST(GetDC(window));
        screen_buffer_blit(window_dc, g_screen_buffer);
        ReleaseDC(window, window_dc);

        DEBUG_PRINT(
            std::format("{} ns\n", stopwatch.split().nanosecond_value()).c_str()
        );

        // Sleep(500);
    }

    return 0;
}
