#pragma once

#import <Metal/Metal.h>

#include "spargel/runtime/gpu/context.h"

namespace spargel::runtime::gpu {
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
}  // namespace spargel::runtime::gpu
