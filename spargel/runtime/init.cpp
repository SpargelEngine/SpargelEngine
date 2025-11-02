#include "spargel/runtime/init.h"

#include "spargel/runtime/gpu/context.h"
#include "spargel/runtime/ui/context.h"

namespace spargel {

void init_runtime() {
  gpu::Context::get().init();
  ui::Context::get().init();
}

}  // namespace spargel
