#include "spargel/base/command_line.h"
#include "spargel/base/trace.h"
#include "spargel/json/json_parser.h"
#include "spargel/json/json_value.h"
#include "spargel/resource/directory.h"

/* libc */
#include <stdio.h>

using namespace spargel;
using namespace spargel::json;
using namespace spargel::base::literals;

namespace {

    void dumpValue(const JsonValue& value);

    void dumpObject([[maybe_unused]] const JsonObject& object) {
        printf("{TODO}");
    }

    void dumpArray(const JsonArray& array) {
        putchar('[');
        putchar(' ');
        usize count = array.elements.count();
        for (usize i = 0; i < count; i++) {
            dumpValue(array.elements[i]);
            if (i < count - 1) {
                putchar(',');
                putchar(' ');
            }
        }
        putchar(' ');
        putchar(']');
    }

    void dumpString(const JsonString& string) {
        putchar('"');
        printf("%s", string.data());
        putchar('"');
    }

    void dumpValue(const JsonValue& value) {
        switch (value.type) {
        case JsonValueType::object:
            dumpObject(value.object);
            break;
        case JsonValueType::array:
            dumpArray(value.array);
            break;
        case JsonValueType::string:
            dumpString(value.string);
            break;
        case JsonValueType::number:
            printf("%lf", value.number);
            break;
        case JsonValueType::boolean:
            printf(value.boolean ? "true" : "false");
            break;
        case JsonValueType::null:
            printf("null");
            break;
        }
    }

}  // namespace

int main(int argc, char* argv[]) {
    spargel_trace_scope("main");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    base::CommandLine cmdline{argc, argv};

    auto manager = resource::ResourceManagerDirectory(""_sv);
    auto optional = manager.open(resource::ResourceId(argv[1]));
    if (!optional.hasValue()) {
        fprintf(stderr, "Cannot open file \"%s\"\n", argv[1]);
        return 1;
    }
    auto resource = base::move(optional.value());

    auto result =
        JsonParser::parse((char*)resource->mapData(), resource->size());
    if (result.isLeft()) {
        if (cmdline.hasSwitch("no-dump")) {
            return 0;
        }
        dumpValue(base::move(result.left()));
        putchar('\n');
    } else {
        fprintf(stderr, "Failed to parse JSON: %s\n",
                base::CString(result.right().message()).data());
    }

    return 0;
}
