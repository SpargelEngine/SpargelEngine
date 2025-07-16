
#include <spargel/base/logging.h>
#include <spargel/base/platform.h>
#include <spargel/entry/simple.h>
#include <spargel/resource/resource.h>
#include <spargel/util/path.h>

// libc
#include <stdio.h>

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

namespace spargel::entry {

    int simple_entry(simple_entry_data* entry_data) {
        base::StringView path = "data.txt"_sv;

        auto resource = entry_data->resource_manager->open(resource::ResourceId(path));
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

}  // namespace spargel::entry
