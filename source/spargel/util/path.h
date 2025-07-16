#pragma once

#include <spargel/base/string.h>
#include <spargel/base/vector.h>

namespace spargel::util {

    struct ParsedPath {
        // whether the path starts with '/'
        bool absolute;

        // whether the path ends with '/
        bool directory;

        base::vector<base::String> components;
    };

    base::String dirname(const base::String& path);

    ParsedPath parsePath(const base::String& path);

}  // namespace spargel::util
