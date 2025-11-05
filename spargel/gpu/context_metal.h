#pragma once

#import <Metal/Metal.h>

#include "spargel/gpu/context.h"

namespace spargel::gpu {

class ContextMetal final : public Context {
public:
  ~ContextMetal();

  void init() override;

  id<MTLDevice> device();
  id<MTLCommandQueue> queue();

private:
  id<MTLDevice> device_ = nullptr;
  id<MTLCommandQueue> queue_ = nullptr;
};

}  // namespace spargel::gpu
