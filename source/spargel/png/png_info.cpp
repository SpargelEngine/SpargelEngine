#include "spargel/logging/logging.h"
#include "spargel/png/png_decoder.h"
#include "spargel/resource/directory.h"

//
#include <stdio.h>

using namespace spargel;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    auto manager = resource::ResourceManagerDirectory("");
    auto optional = manager.open(resource::ResourceId(argv[1]));
    if (!optional.hasValue()) {
        logging::info("cannot open file `{}`\n", argv[1]);
        return 1;
    }
    auto resource = base::move(optional.value());

    png::PNGDecoder decoder{resource->getSpan()};

    auto status = decoder.decode();

    printf("status: %d\n", (int)status);

    return 0;
}
