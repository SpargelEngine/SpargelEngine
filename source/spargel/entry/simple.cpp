#include <spargel/base/string.h>
#include <spargel/entry/simple.h>
#include <spargel/resource/directory.h>
#include <spargel/ui/platform.h>
#include <spargel/ui/window.h>

int main() {
    spargel::entry::simple_entry_data data;
    data.platform = spargel::ui::makePlatform();
    data.window = data.platform->makeWindow(500, 500);
    data.resource_manager = spargel::resource::make_relative_manager();

    int ret = spargel::entry::simple_entry(&data);

    data.resource_manager->close();

    return ret;
}
