#include "spargel/base/allocator.h"
#include "spargel/base/object.h"
#include "spargel/base/unique_ptr.h"
#include "spargel/entry/component.h"
#include "spargel/entry/launch_data.h"
#include "spargel/logging/logging.h"
#include "spargel/ui/window.h"

using namespace spargel;

#define DEMO_EXPORT [[gnu::visibility("default")]]

class DEMO_EXPORT demo_component final : public entry::Component {
public:
    explicit demo_component(entry::LaunchData* l) : entry::Component(l) {}

    void onLoad() override {
        _window = getLaunchData()->platform->makeWindow(500, 500);
        _window->setTitle("Spargel - Component Demo");
        spargel_log_info("demo component loaded");
    }

private:
    base::unique_ptr<ui::Window> _window;
};

extern "C" DEMO_EXPORT entry::Component* _spargel_make_component(
    entry::LaunchData* l) {
    return base::default_allocator()->allocObject<demo_component>(l);
}
