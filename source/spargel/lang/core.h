#pragma once

#include <spargel/base/vector.h>

namespace spargel::lang::core {

    class Argument;
    class BasicBlock;
    class Function;
    class Instruction;

    //==================
    // class BasicBlock
    //
    // A basic block is an array of instructions executed sequentially.
    //
    // Note:
    //   - Basic block arguments instead of phi nodes (e.g. LLVM).
    //
    class BasicBlock {
    private:
        // The function containing this basic block.
        Function* _func;

        // TODO: Use SmallArray instead.
        base::vector<Argument*> _args;

        base::vector<Instruction> _insts;
    };

    //--------------
    // Instructions
    //
    //  - `alloc_stack` : allocate memory on the stack
    //  - `load` : read from memory
    //  - `store` : store to memory
    //  - `return` : return from a function
    //  - `branch` : transfer control flow unconditionally
    //  - `cond_branch` : transfer control flow conditionally
    //  - `integer_literal`

}
