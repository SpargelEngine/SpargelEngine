#include "spargel/logging/logging.h"
#include "spargel/resource/directory.h"

// libc
#include <stdio.h>

using namespace spargel;
using namespace spargel::base::literals;

static void put_hex(u8 ch) {
    const char hex_digits[] = "0123456789abcdef";
    putchar(hex_digits[ch >> 4]);
    putchar(hex_digits[ch & 0xf]);
}

static void xxd_print(char* data, size_t size) {
    int last_line = (size + 1) / 16;
    for (int i = 0; i <= last_line; i++) {
        printf("%08x: ", 16 * i);
        int cnt = i == last_line ? size - 16 * i : 16;
        for (int j = 0; j < 16; j++) {
            if (j < cnt) {
                put_hex(data[16 * i + j]);
            } else {
                putchar(' ');
                putchar(' ');
            }
            if (j & 0x1) putchar(' ');
        }

        putchar(' ');

        for (int j = 0; j < cnt; j++) {
            char ch = data[16 * i + j];
            if (ch >= ' ' && ch <= '~')
                putchar(ch);
            else
                putchar('.');
        }

        putchar('\n');
    }
}

int main() {
    auto resource_manager =
        resource::makeRelativeManager(base::String("resources"));

    base::StringView path = "dir/abc.txt"_sv;

    auto resource = resource_manager->open(resource::ResourceId(path));
    if (!resource.hasValue()) {
        spargel_log_error("Cannot open resource \"%s\"", path.data());
        return 1;
    }

    size_t size = resource.value()->size();
    spargel_log_info("Size of \"%s\": %zu", path.data(), size);
    spargel_log_info("================");

    char* data = (char*)resource.value()->mapData();
    xxd_print(data, size);

    return 0;
}
