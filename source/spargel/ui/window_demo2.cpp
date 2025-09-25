#include "spargel/entry/simple.h"
#include "spargel/logging/logging.h"
#include "spargel/ui/window.h"

namespace spargel::entry {

    class delegate final : public ui::WindowDelegate {};

    int simple_entry(simple_entry_data* data) {
        spargel_log_debug("window_demo2");
        data->window->setTitle("Spargel - Window Demo 2");
        delegate d;
        data->window->setDelegate(&d);
        data->platform->startLoop();
        return 0;
    }

}  // namespace spargel::entry
