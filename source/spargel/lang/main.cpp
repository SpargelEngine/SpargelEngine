#include "spargel/lang/lexer.h"
#include "spargel/lang/parser.h"

namespace spargel::lang {
    namespace {
        void DriverMain() {
            Cursor c{R"(
                open cpp_interop
                func main
            )"};
            Parser p{c};
            auto result = p.parse();
            result.dump();
        }
    }  // namespace
}  // namespace spargel::lang

int main() {
    spargel::lang::DriverMain();
    return 0;
}
