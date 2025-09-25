#include "spargel/ui/platform_win32.h"

#include <Windowsx.h>

#include "spargel/logging/logging.h"
#include "spargel/ui/window_win32.h"

namespace spargel::ui {

    namespace {
        constexpr auto CLASS_NAME = "Spargel Engine Window Class";

        LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam) {
            switch (uMsg) {
            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }
            case WM_LBUTTONDOWN: {
                auto x = GET_X_LPARAM(lParam);
                auto y = GET_Y_LPARAM(lParam);
                spargel_log_info("click: %d, %d", x, y);
                break;
            }
            }
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        }
    }  // namespace

    void PlatformWin32::startLoop() {
        MSG msg = {};
        // TODO: Poll for events.
        while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    base::unique_ptr<Window> PlatformWin32::makeWindow(u32 width, u32 height) {
        WNDCLASS wc = {};

        wc.lpfnWndProc = windowProc;
        wc.hInstance = hInstance_;
        wc.lpszClassName = CLASS_NAME;

        RegisterClassA(&wc);

        HWND hwnd = CreateWindowExA(0,                 // optional window styles
                                    CLASS_NAME,        // window class
                                    "Spargel Engine",  // window name
                                    WS_OVERLAPPEDWINDOW,  // window style

                                    // size and position
                                    CW_USEDEFAULT, CW_USEDEFAULT, (int)width,
                                    (int)height,

                                    nullptr,     // parent window
                                    nullptr,     // menu
                                    hInstance_,  // instance handle
                                    nullptr      // additional application data
        );

        if (hwnd == nullptr) {
            spargel_log_error("CreateWindowEx failed");
            return nullptr;
        }

        ShowWindow(hwnd, SW_SHOWNORMAL);

        auto window = base::makeUnique<WindowWin32>(this, hwnd);
        windows_.push(window.get());

        return window;
    }

    base::unique_ptr<Platform> makePlatformWin32() {
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandleA(nullptr);
        return base::makeUnique<PlatformWin32>(hInstance);
    }

}  // namespace spargel::ui
