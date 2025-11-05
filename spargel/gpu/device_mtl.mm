#include "spargel/gpu/device_mtl.h"

#include "spargel/base/check.h"

namespace spargel::gpu {

DeviceMTL::DeviceMTL(id<MTLDevice> device) : device_{device} {}
DeviceMTL::~DeviceMTL() { device_ = nullptr; }

CommandQueue* DeviceMTL::createCommandQueue() {
  id<MTL4CommandQueue> queue = [device_ newMTL4CommandQueue];
  CHECK(queue);
  return new CommandQueueMTL(queue);
}

CommandBuffer* DeviceMTL::createCommandBuffer() { return nullptr; }

Buffer* DeviceMTL::createBuffer() { return nullptr; }
Texture* DeviceMTL::createTexture() { return nullptr; }

ComputePipeline* DeviceMTL::createComputePipeline() { return nullptr; }
ComputeEncoder* DeviceMTL::createComputeEncoder() { return nullptr; }

CommandQueueMTL::CommandQueueMTL(id<MTL4CommandQueue> queue) : queue_{queue} {}

CommandQueueMTL::~CommandQueueMTL() { queue_ = nullptr; }

void CommandQueueMTL::submit(CommandBuffer* /*cmdbuf*/) {}

}  // namespace spargel::gpu
