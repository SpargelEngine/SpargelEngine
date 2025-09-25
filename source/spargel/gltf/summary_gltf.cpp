#include "spargel/gltf/gltf.h"
#include "spargel/resource/directory.h"

/* libc */
#include <stdio.h>

namespace spargel::gltf {
    namespace {
        using namespace spargel::base::literals;

        void dumpGlTF(const GlTF& gltf) {
            // asset
            auto& asset = gltf.asset;
            puts("asset:");
            printf("  version: \"%s\"\n", base::CString(asset.version).data());
            if (asset.copyright.hasValue())
                printf("  copyright: \"%s\"\n",
                       base::CString(asset.copyright.value()).data());
            if (asset.generator.hasValue())
                printf("  generator: \"%s\"\n",
                       base::CString(asset.generator.value()).data());
            if (asset.minVersion.hasValue())
                printf("  minVersion: \"%s\"\n",
                       base::CString(asset.minVersion.value()).data());

            if (gltf.accessors.hasValue())
                printf("accessors: %zu\n", gltf.accessors.value().count());

            if (gltf.buffers.hasValue())
                printf("buffers: %zu\n", gltf.buffers.value().count());

            if (gltf.bufferViews.hasValue())
                printf("bufferViews: %zu\n", gltf.bufferViews.value().count());

            if (gltf.meshes.hasValue())
                printf("meshes: %zu\n", gltf.meshes.value().count());

            if (gltf.nodes.hasValue())
                printf("nodes: %zu\n", gltf.nodes.value().count());

            if (gltf.scenes.hasValue())
                printf("scenes: %zu\n", gltf.scenes.value().count());

            if (gltf.scene.hasValue())
                printf("scene: %d\n", gltf.scene.value());
        }

        int main(int argc, char* argv[]) {
            if (argc < 2) {
                fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
                return 1;
            }

            auto manager = resource::ResourceManagerDirectory(""_sv);
            auto optional = manager.open(resource::ResourceId(argv[1]));
            if (!optional.hasValue()) {
                fprintf(stderr, "Cannot open file \"%s\"\n", argv[1]);
                return 1;
            }
            auto& resource = optional.value();

            auto result =
                parseGlTF((char*)resource->mapData(), resource->size());
            if (result.isLeft()) {
                dumpGlTF(result.left());
                putchar('\n');
            } else {
                fprintf(stderr, "Failed to parse glTF: %s\n",
                        base::CString(result.right().message()).data());
            }

            return 0;
        }
    }  // namespace

}  // namespace spargel::gltf

int main(int argc, char* argv[]) { return spargel::gltf::main(argc, argv); }
