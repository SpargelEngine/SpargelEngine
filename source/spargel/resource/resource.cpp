#include "spargel/resource/resource.h"

#include "spargel/logging/logging.h"

namespace spargel::resource {

    Resource::~Resource() {
        if (_mapped) {
            base::default_allocator()->free(_mapped, _mapped_size);
        }
    }

    void* Resource::mapData() {
        if (!_mapped) {
            _mapped_size = size();
            _mapped = base::default_allocator()->allocate(_mapped_size);
            getData(_mapped);
        }
        return _mapped;
    }

}  // namespace spargel::resource
