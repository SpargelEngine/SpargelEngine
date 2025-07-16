#include <spargel/base/deflate.h>
#include <spargel/base/string.h>
#include <spargel/base/string_view.h>
#include <spargel/base/vector.h>
#include <spargel/resource/directory.h>

//
#include <stdio.h>
#include <string.h>

namespace spargel::base {
    namespace {
        using namespace literals;
        int DeflateMain(Vector<StringView> const& args) {
            if (args.count() == 0) {
                printf("usage: deflate_demo <file>\n");
                return 1;
            }
            // TODO: Remove hardcoded path.
            auto resource_manager = resource::ResourceManagerDirectory("."_sv);
            auto resource = resource_manager.open(resource::ResourceId(args[0]));
            if (!resource.hasValue()) {
                return 1;
            }
            auto data = resource.value()->getSpan();
            Vector<Byte> result;
            DeflateDecompressor decomp;
            decomp.decompress(data, result);
            printf("%s\n", CString{(char*)result.begin(), (char*)result.end()}.data());
            return 0;
        }
    }  // namespace
}  // namespace spargel::base

int main(int argc, char const* argv[]) {
    spargel::base::Vector<spargel::base::StringView> args;
    for (int i = 1; i < argc; i++) {
        args.emplace(argv[i], strlen(argv[i]));
    }
    return spargel::base::DeflateMain(args);
}
