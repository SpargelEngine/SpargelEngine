#pragma once

// The immediate mode GUI framework.
//

namespace spargel::ui::im {

    // The big context.
    class ImCx {
    public:
    private:
    };

    // An area on the screen that can be dragged.
    //
    // ```
    // Area("my_area")
    //     .init_pos(10.0, 10.0)
    //     .show(cx, [](ImCx& cx) {
    //         cx.label("my area");
    //     });
    // ```
    class Area {};

}  // namespace spargel::ui::im
