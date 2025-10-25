#include "spargel/runtime/gpu/context_metal.h"

#include "spargel/runtime/logging.h"

namespace spargel::runtime::gpu {
Context& Context::get() {
    static ContextMetal mtl_ctx;
    return mtl_ctx;
}
ContextMetal::~ContextMetal() {
    queue_ = nullptr;
    device_ = nullptr;
}
void ContextMetal::init() {
    LOG_INFO("init gpu");
    device_ = MTLCreateSystemDefaultDevice();
    queue_ = [device_ newCommandQueue];
}
id<MTLDevice> ContextMetal::device() { return device_; }
id<MTLCommandQueue> ContextMetal::queue() { return queue_; }
}  // namespace spargel::runtime::gpu
