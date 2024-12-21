#pragma once

#include <spargel/base/vector.h>
#include <spargel/ui/ui.h>

/* xcb */
#include <xcb/xcb.h>

namespace spargel::ui {

    class window_xcb;

    class platform_xcb : public platform {
        friend window_xcb;

    public:
        platform_xcb();
        ~platform_xcb() override;

        void start_loop() override;

        base::unique_ptr<window> make_window(u32 width, u32 height) override;
        base::unique_ptr<TextSystem> createTextSystem() override { return nullptr; }

    private:
        xcb_connection_t* _connection;
        xcb_screen_t* _screen;

        base::vector<window_xcb*> _windows;

        void _run_render_callbacks();
    };

    base::unique_ptr<platform> make_platform_xcb();

    class window_xcb : public window {
        friend platform_xcb;

    public:
        window_xcb(platform_xcb* platform, xcb_window_t id);
        ~window_xcb() override;

        void set_title(char const* title) override;

        window_handle handle() override;

    private:
        platform_xcb* _platform;
        xcb_window_t _id;
    };

}  // namespace spargel::ui
