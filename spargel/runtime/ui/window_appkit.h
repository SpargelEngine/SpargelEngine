#pragma once

#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>

#include "spargel/runtime/ui/window.h"

@interface SpargelApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@interface SpargelWindowDelegate : NSObject <NSWindowDelegate>
@end

@interface SpargelViewController : NSViewController <MTKViewDelegate>
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

    MTKView* mtk_view() { return view_; }

    void set_view(MTKView* view) { view_ = view; }

private:
    void init_app();
    void init_window();

    NSApplication* ns_app_ = nullptr;
    SpargelApplicationDelegate* nsapp_delegate_ = nullptr;
    NSWindow* ns_window_ = nullptr;
    MTKView* view_ = nullptr;
    SpargelViewController* view_controller_ = nullptr;
};
}  // namespace spargel::runtime::ui
