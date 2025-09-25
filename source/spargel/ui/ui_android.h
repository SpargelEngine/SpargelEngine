#pragma once

#include "spargel/base/base.h"
#include "spargel/logging/logging.h"
#include "spargel/ui/ui.h"

// Android
#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace spargel::ui {
    class platform_android;
    class window_android;

    struct android_app_data {
        struct android_app* app;
        platform_android* platform;
        bool can_render;
    };

    class platform_android : public platform {
        friend window_android;

    public:
        explicit platform_android(struct android_app* app);
        ~platform_android() override;

        void start_loop() override;

        base::unique_ptr<window> make_window(u32 width, u32 height) override {
            spargel_log_fatal(
                "platform_android::make_window should not be called");
            spargel_panic_here();
        }
        base::unique_ptr<TextSystem> createTextSystem() override {
            return nullptr;
        }

        base::unique_ptr<window_android> generate_window_handle();

        window_android* window_handle() { return window; }

    private:
        struct android_app* _app;
        window_android* window;
    };

    class window_android : public window {
        friend platform_android;

    public:
        explicit window_android(platform_android& platform);
        ~window_android() override;

        void set_title(char const* title) override;

        void setAnimating(bool animating) override {}

        void requestRedraw() override {}

        window_handle handle() override;

    private:
        platform_android& _platform;
    };

}  // namespace spargel::ui
