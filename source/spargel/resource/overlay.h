#pragma once

#include "spargel/base/unique_ptr.h"
#include "spargel/base/vector.h"
#include "spargel/resource/resource.h"

namespace spargel::resource {

    /*
     * A stack of resource managers.
     *
     * They are stacked from the rightmost, with the leftmost being the top.
     * TODO: whiteout, opaque
     */
    class ResourceManagerOverlay : public ResourceManager {
    public:
        ResourceManagerOverlay(
            base::Vector<base::unique_ptr<ResourceManager>>&& sub_managers)
            : _sub_managers(base::move(sub_managers)) {}

        bool has(const ResourceId& id) override;

        base::Optional<base::unique_ptr<Resource>> open(
            const ResourceId& id) override;

    private:
        base::Vector<base::unique_ptr<ResourceManager>> _sub_managers;
    };

}  // namespace spargel::resource
