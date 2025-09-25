#include "spargel/base/base.h"
#include "spargel/entry/simple.h"
#include "spargel/logging/logging.h"
#include "spargel/resource/android_asset.h"
#include "spargel/ui/ui_android.h"

// JNI & Android
#include <jni.h>

// Game Activity
#include <game-activity/native_app_glue/android_native_app_glue.h>

using namespace spargel;

static void onAppCmd(struct android_app* app, int32_t cmd) {
    auto* data = (ui::android_app_data*)app->userData;
    auto* platform = data->platform;
    auto* window = platform->window_handle();

    switch (cmd) {
    case APP_CMD_START:
        break;
    case APP_CMD_INIT_WINDOW:
        data->can_render = true;
        break;
    case APP_CMD_TERM_WINDOW:
        if (window) window->delegate()->on_close_requested();
        data->can_render = false;
        break;
    case APP_CMD_DESTROY:
        if (window) window->delegate()->on_closed();
        break;
    default:
        break;
    }
}

extern "C" {
void android_main(struct android_app* app);
}

void android_main(struct android_app* app) {
    volatile auto* data = new ui::android_app_data;
    data->app = app;
    data->can_render = false;
    app->userData = (void*)data;
    app->onAppCmd = onAppCmd;

    entry::simple_entry_data entry_data;
    auto platform = base::make_unique<ui::platform_android>(app);
    entry_data.window = platform->generate_window_handle();
    entry_data.platform = base::move(platform);
    entry_data.resource_manager =
        base::make_unique<resource::android_asset_resource_manager>(
            app->activity->assetManager);

    // FIXME: wait until surface is ready
    while (!app->destroyRequested) {
        android_poll_source* source;
        auto result = ALooper_pollOnce(-1, nullptr, nullptr, (void**)&source);
        if (result == ALOOPER_POLL_ERROR) {
            spargel_log_error("ALooper_pollOnce returned an error");
            spargel_panic_here();
        }

        if (source != nullptr) {
            source->process(app, source);
        }

        if (data->can_render) break;
    }

    simple_entry(&entry_data);

    entry_data.resource_manager->close();

    // empty loop
    while (!app->destroyRequested) {
        android_poll_source* source;
        auto result = ALooper_pollOnce(-1, nullptr, nullptr, (void**)&source);
        if (result == ALOOPER_POLL_ERROR) {
            spargel_log_error("ALooper_pollOnce returned an error");
            spargel_panic_here();
        }

        if (source != nullptr) {
            source->process(app, source);
        }
    }

    delete data;
}
