#pragma once

#include "spargel/base/span.h"
#include "spargel/base/string_view.h"
#include "spargel/base/vector.h"

//
#include <string.h>

namespace spargel::base {

    // TODO: Redesign.
    //
    class CommandLine {
    public:
        explicit CommandLine(int argc, char* argv[])
            : _args(argv + 1, argv + argc) {
            parse();
        }

        bool hasSwitch(string_view name) {
            for (auto s : _switches) {
                if (s == name) {
                    return true;
                }
            }
            return false;
        }

    private:
        void parse() {
            for (auto arg : _args) {
                usize len = strlen(arg);
                if (len < 2) {
                    // not a switch
                    continue;
                }
                if (memcmp(arg, "--", 2) == 0) {
                    // arg is a switch
                    _switches.emplace(arg + 2, arg + len);
                }
            }
        }

        Span<char*> _args;
        Vector<StringView> _switches;
    };

}  // namespace spargel::base
