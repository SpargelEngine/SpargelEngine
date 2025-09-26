#include "spargel/cocoa/display_modes.h"

#include <CoreGraphics/CoreGraphics.h>

namespace spargel::cocoa {
    void DisplayMode::query(base::Vector<DisplayMode>& modes) {
        auto modes_array =
            CGDisplayCopyAllDisplayModes(CGMainDisplayID(), nullptr);
        struct CallData {
            base::Vector<DisplayMode>& modes;
        } data = {modes};
        CFArrayApplyFunction(
            modes_array, CFRangeMake(0, CFArrayGetCount(modes_array)),
            [](void const* value, void* data) {
                auto mode = CGDisplayModeRef(value);
                auto width = CGDisplayModeGetWidth(mode);
                auto height = CGDisplayModeGetHeight(mode);
                auto call_data = static_cast<CallData*>(data);
                call_data->modes.emplace(width, height);
            },
            &data);
        CFRelease(modes_array);
    }

}  // namespace spargel::cocoa
