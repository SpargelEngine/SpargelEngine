#pragma once

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "spargel/runtime/ui/window.h"

@interface SpargelApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@interface SpargelWindowDelegate : NSObject <NSWindowDelegate>
@end

// TODO(tianjiao): implement <NSTextInputClient>
@interface SpargelMainView : NSView
@end

namespace spargel::runtime::ui {
    class WindowAppKit final : public Window {
    public:
        WindowAppKit() {
            init_app();
            init_window();
        }

        void set_title(char const* title) override;
        void show() override { [NSApp run]; }

        CAMetalLayer* metal_layer() { return metal_layer_; }

    private:
        void init_app();
        void init_window();

        NSApplication* ns_app_ = nullptr;
        SpargelApplicationDelegate* nsapp_delegate_ = nullptr;
        NSWindow* ns_window_ = nullptr;
        CAMetalLayer* metal_layer_ = nullptr;
        SpargelMainView* main_view_ = nullptr;
    };
}  // namespace spargel::runtime::ui
