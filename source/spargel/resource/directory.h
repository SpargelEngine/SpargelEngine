#pragma once

#include "spargel/base/unique_ptr.h"
#include "spargel/resource/resource.h"

namespace spargel::resource {

    class ResourceManagerDirectory : public ResourceManager {
    public:
        ResourceManagerDirectory(base::StringView root_path)
            : _root_path(root_path) {}

        bool has(const ResourceId& id) override;

        base::Optional<base::unique_ptr<Resource>> open(
            const ResourceId& id) override;

    private:
        base::String _root_path;

        base::String _real_path(const ResourceId& id);
    };

    base::unique_ptr<ResourceManagerDirectory> makeRelativeManager(
        const base::String& resources_dir = base::String());

}  // namespace spargel::resource
