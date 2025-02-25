#include <spargel/base/allocator.h>
#include <spargel/base/logging.h>
#include <spargel/base/object.h>
#include <spargel/base/unique_ptr.h>
#include <spargel/entry/component.h>
#include <spargel/entry/launch_data.h>
#include <spargel/ui/window.h>

using namespace spargel;

#define DEMO_EXPORT [[gnu::visibility("default")]]

class DEMO_EXPORT demo_component final : public entry::component {
public:
    explicit demo_component(entry::launch_data* l) : entry::component(l) {}

    void on_load() override {
        _window = get_launch_data()->platform->makeWindow(500, 500);
        _window->setTitle("Spargel - Component Demo");
        spargel_log_info("demo component loaded");
    }

private:
    base::unique_ptr<ui::Window> _window;
};

extern "C" DEMO_EXPORT entry::component* _spargel_make_component(entry::launch_data* l) {
    return base::default_allocator()->alloc_object<demo_component>(l);
}
