#include "spargel/base/string.h"
#include "spargel/logging/logging.h"
#include "spargel/resource/directory.h"
#include "spargel/resource/overlay.h"

using namespace spargel;
using namespace spargel::base::literals;

// libc
#include <stdio.h>

int main() {
    base::vector<base::unique_ptr<resource::ResourceManager>> managers;
    managers.emplace(resource::makeRelativeManager(base::String("resources2")));
    managers.emplace(resource::makeRelativeManager(base::String("resources")));
    auto manager = resource::ResourceManagerOverlay(base::move(managers));

    base::StringView path1 = "dir/abc.txt"_sv;

    auto resource1 = manager.open(resource::ResourceId(path1));
    if (!resource1.hasValue()) {
        spargel_log_error("Cannot open resource \"%s\"", path1.data());
        return 1;
    }

    spargel_log_info("%s (%zu): ", path1.data(), resource1.value()->size());
    char* data1 = (char*)resource1.value()->mapData();
    for (usize i = 0; i < resource1.value()->size(); i++) putchar(data1[i]);
    putchar('\n');

    base::StringView path2 = "hello.txt"_sv;

    auto resource2 = manager.open(resource::ResourceId(path2));
    if (!resource2.hasValue()) {
        spargel_log_error("Cannot open resource \"%s\"", path2.data());
        return 1;
    }

    spargel_log_info("%s (%zu): ", path2.data(), resource2.value()->size());
    char* data2 = (char*)resource2.value()->mapData();
    for (usize i = 0; i < resource2.value()->size(); i++) putchar(data2[i]);
    putchar('\n');

    return 0;
}
