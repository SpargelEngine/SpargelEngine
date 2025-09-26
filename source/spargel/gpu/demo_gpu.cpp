#include "spargel/gpu/gpu.h"

namespace spargel::gpu {
    namespace {
        void demoMain() {
            auto device = Device::create(DeviceKind::metal);
            device->createShaderFunction("/shaders/demo/triangle_vert");
        }
    }  // namespace
}  // namespace spargel::gpu

int main() {
    spargel::gpu::demoMain();
    return 0;
}
