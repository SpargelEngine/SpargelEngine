#pragma once

#import <Metal/Metal.h>

#include "spargel/gpu/device.h"

namespace spargel::gpu {

class DeviceMTL;
class CommandQueueMTL;
class CommandBufferMTL;

class DeviceMTL final : public Device {
public:
  DeviceMTL(id<MTLDevice> device);
  ~DeviceMTL();

  CommandQueue* createCommandQueue() override;
  CommandBuffer* createCommandBuffer() override;

  Buffer* createBuffer() override;
  Texture* createTexture() override;

  ComputePipeline* createComputePipeline() override;
  ComputeEncoder* createComputeEncoder() override;

private:
  id<MTLDevice> device_ = nullptr;
};

class CommandQueueMTL final : public CommandQueue {
public:
  CommandQueueMTL(id<MTL4CommandQueue> queue);
  ~CommandQueueMTL();

  void submit(CommandBuffer* cmdbuf) override;

private:
  id<MTL4CommandQueue> queue_ = nullptr;
};

class CommandBufferMTL final : public CommandBuffer {
public:
  CommandBufferMTL();
  ~CommandBufferMTL();

private:
  id<MTL4CommandBuffer> cmdbuf_ = nullptr;
  // > Command allocators only service a single command buffer at a time.
  id<MTL4CommandAllocator> alloc_ = nullptr;
};

}  // namespace spargel::gpu
