#pragma once

#include "spargel/base/span.h"
#include "spargel/base/vector.h"
#include "spargel/math/rectangle.h"
#include "spargel/ui/event.h"

namespace spargel::gpu {
    class GPUContext;
}

namespace spargel::render {
    class UIScene;
}

namespace spargel::ui {

    class View;
    class ViewHost;

    struct PaintContext {
        gpu::GPUContext* gpu_context;
        render::UIScene* scene;
    };

    // A View corresponds to a complete functionality of the UI.
    //
    // The View class should be sub-classed to provide custom behaviour.
    //
    class View {
    public:
        View() = default;
        virtual ~View();

        // Host is set for every view in the hierarchy.
        ViewHost* host() { return host_; }
        void setHost(ViewHost* host);

        bool isRootView() const { return parent_ == nullptr; }

        // View Hierarchy
        // --------------
        //
        // Every view owns its children.
        //

        base::Span<View*> children() const { return children_.toSpan(); }

        View* childAt(usize i) { return children_[i]; }

        // The ownership is taken by the view.
        void addChild(View* v);
        // Construct a child view at back.
        template <typename T, typename... Args>
        T* emplaceChild(Args&&... args) {
            auto ptr = new T(base::forward<Args>(args)...);
            addChild(ptr);
            return ptr;
        }

        // The ownership of the removed child is handled over to the caller.
        [[nodiscard]]
        View* removeChild(usize i);

        View* parent() { return parent_; }
        // This should only be called by the parent.
        void setParent(View* p) { parent_ = p; }

        // Positioning
        // -----------

        // The bounding box in parent's coordinate system.
        math::Rect frame() const { return frame_; }
        void setFrame(math::Rect frame) { frame_ = frame; }
        void setFrame(float x, float y, float w, float h) {
            setFrame(math::Rect{{x, y}, {w, h}});
        }

        float width() const { return frame_.size.width; }
        float height() const { return frame_.size.height; }

        // Layout
        // ------
        //
        // Every view manages the positions of its children.
        // The layout of the children is determined via a negotiation process.

        // How much space this view needs given the proposal.
        virtual math::RectSize getPreferredSize(
            math::RectSize proposal) {
            return proposal;
        }
        // Put the children at correct places.
        virtual void placeChildren() {}

        void invalidateLayout();

        bool needsLayout() const { return needs_layout_; }

        // Paint
        // -----

        // Paint the view.
        // NOTE: This is called by the framework.
        void paint(PaintContext& context);

        // Ask the ViewHost to schedule a repaint.
        void requestRepaint();

        // Event Handling
        // --------------
        //
        // Events received are in the parent coordinate.
        //

        void handleMousePressed(MouseDownEvent const& e);
        void handleMouseReleased();
        void handleMouseMoved(const MouseMovedEvent& e);

        virtual void onMouseDown([[maybe_unused]] float x,
                                 [[maybe_unused]] float y) {}
        virtual void onMouseMoved([[maybe_unused]] const MouseMovedEvent& e) {}

        // Record painting commands to the scene.
        virtual void onPaint([[maybe_unused]] render::UIScene& scene) {}

        // Bypass UIRenderer.
        // virtual void onPaint([[maybe_unused]] gpu::GPUContext& ctx) {}

    private:
        ViewHost* host_ = nullptr;
        View* parent_ = nullptr;
        base::Vector<View*> children_;

        math::Rect frame_;
        // Offset in parent's coordinate.
        // math::Vector2f offset_;

        bool needs_layout_ = true;
    };
}  // namespace spargel::ui
