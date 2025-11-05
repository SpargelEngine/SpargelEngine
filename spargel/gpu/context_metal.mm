#include "spargel/gpu/context_metal.h"

#include "spargel/base/logging.h"

namespace spargel::gpu {

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

}  // namespace spargel::gpu
