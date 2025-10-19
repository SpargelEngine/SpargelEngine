#pragma once

#include "spargel/base/bit_cast.h"
#include "spargel/base/enum.h"
#include "spargel/base/types.h"
#include "spargel/base/vector.h"
#include "spargel/math/function.h"
#include "spargel/math/rectangle.h"
#include "spargel/render/ui_renderer.h"
#include "spargel/text/styled_text.h"

namespace spargel::text {
    class Font;
}

namespace spargel::render {
    class UIRenderer;

    // Coordinates: The origin is at top-left.
    //
    // V1: Commands and data are separated.
    //
    // V2: Combined command with a uniform size.
    //
    class UIScene {
    public:
        UIScene() { clear(); }

        void setRenderer(UIRenderer* renderer) { renderer_ = renderer; }

        // TODO: Clip stack.
        void setClip(float x, float y, float w, float h) {
            auto transform = currentTransform();
            x += transform.x;
            y += transform.y;

            pushCommand(DrawCommand::set_clip);
            pushData(x, y, w, h);
            clip_ = math::Rect{{x * scale_, y * scale_},
                                    {w * scale_, h * scale_}};
        }
        void setClip(math::Rect rect) {
            setClip(rect.origin.x, rect.origin.y, rect.size.width,
                    rect.size.height);
        }
        // Warning: The default clip is zero-sized. So nothing will be rendered.
        void clearClip() {
            pushCommand(DrawCommand::clear_clip);
            clip_ = math::Rect{};
        }

        void fillRect(float x, float y, float w, float h, u32 color) {
            auto transform = currentTransform();
            x += transform.x;
            y += transform.y;

            pushCommand(DrawCommand::fill_rect);
            pushData(x, y, w, h, color);
            pushCommand2(DrawCommand::fill_rect, x * scale_, y * scale_,
                         w * scale_, h * scale_, color);

            auto m = (math::ceil(w) + 1.0f) * scale_;
            auto n = (math::ceil(h) + 1.0f) * scale_;
            estimated_slots_ += (usize)((math::ceil(m / 8.0f) + 1.0f) *
                                        (math::ceil(n / 8.0f) + 1.0f));
        }
        void fillCircle(float x, float y, float r, u32 color) {
            auto transform = currentTransform();
            x += transform.x;
            y += transform.y;

            pushCommand(DrawCommand::fill_circle);
            pushData(x, y, r, color);
            pushCommand2(DrawCommand::fill_circle, x * scale_, y * scale_,
                         r * scale_, color);

            auto n = (math::ceil(2.0f * r) + 1.0f) * scale_;
            estimated_slots_ += (usize)((math::ceil(n / 8.0f) + 1.0f) *
                                        (math::ceil(n / 8.0f) + 1.0f));
        }
        // v2 only
        void fillRoundedRect(float x, float y, float w, float h, float r,
                             u32 color) {
            auto transform = currentTransform();
            x += transform.x;
            y += transform.y;

            pushCommand2(DrawCommand::fill_rounded_rect, x * scale_, y * scale_,
                         w * scale_, h * scale_, r * scale_, color);

            auto m = (math::ceil(w) + 1.0f) * scale_;
            auto n = (math::ceil(h) + 1.0f) * scale_;
            estimated_slots_ += (usize)((math::ceil(m / 8.0f) + 1.0f) *
                                        (math::ceil(n / 8.0f) + 1.0f));
        }
        void strokeLine(float x0, float y0, float x1, float y1, u32 color) {
            auto transform = currentTransform();
            x0 += transform.x;
            y0 += transform.y;
            x1 += transform.x;
            y1 += transform.y;

            pushCommand(DrawCommand::stroke_line);
            pushData(x0, y0, x1, y1, color);
            pushCommand2(DrawCommand::stroke_line, x0 * scale_, y0 * scale_,
                         x1 * scale_, y1 * scale_, color);

            auto m = (math::ceil(math::abs(x0 - x1)) + 2.0f) * scale_;
            auto n = (math::ceil(math::abs(y0 - y1)) + 2.0f) * scale_;
            estimated_slots_ += (usize)((math::ceil(m / 8.0f) + 1.0f) *
                                        (math::ceil(n / 8.0f) + 1.0f));
        }
        void strokeCircle(float x, float y, float r, u32 color) {
            auto transform = currentTransform();
            x += transform.x;
            y += transform.y;

            pushCommand(DrawCommand::stroke_circle);
            pushData(x, y, r, color);
            pushCommand2(DrawCommand::stroke_circle, x * scale_, y * scale_,
                         r * scale_, color);

            auto n = (math::ceil(2.0f * r) + 1.0f) * scale_;
            estimated_slots_ += (usize)((math::ceil(n / 8.0f) + 1.0f) *
                                        (math::ceil(n / 8.0f) + 1.0f));
        }
        // Render a line of text with the given font and color.
        //
        // TODO: Support emojis.
        void fillText(text::StyledText text, float x, float y, u32 color);

        void strokeRectangle(float x, float y, float w, float h, u32 color) {
            strokeLine(x, y, x + w, y, color);
            strokeLine(x + w, y, x + w, y + h, color);
            strokeLine(x + w, y + h, x, y + h, color);
            strokeLine(x, y + h, x, y, color);
        }
        void strokeRectangle(math::Rect rect, u32 color) {
            strokeRectangle(rect.origin.x, rect.origin.y, rect.size.width,
                            rect.size.height, color);
        }

        void dump() { pushCommand(DrawCommand::dump); }

        base::Span<u8> commands() const { return commands_.toSpan(); }
        base::Span<u32> data() const { return data_.toSpan(); }

        base::Span<base::Byte> commands2_bytes() const {
            return commands2_.toSpan().asBytes();
        }
        usize commands2_count() const { return commands2_.count(); }

        void clear() {
            commands_.clear();
            data_.clear();
            commands2_.clear();
            transform_stack_.clear();
            transform_stack_.emplace(0.0f, 0.0f);
            estimated_slots_ = 0;
        }

        void setScale(float s) { scale_ = s; }

        void pushTransform(float dx, float dy) {
            auto current = currentTransform();
            transform_stack_.emplace(current.x + dx, current.y + dy);
        }

        void popTransform() { transform_stack_.pop(); }

        usize estimatedSlots() const { return estimated_slots_; }

    private:
        enum class DrawCommand : u8 {
            fill_rect = 0,
            fill_circle,
            fill_rounded_rect,
            stroke_line,
            stroke_circle,
            // not used currently
            set_clip,
            clear_clip,
            // for glyphs
            sample_texture,
            //
            dump,
        };

        // TODO: 15 bytes are wasted.
        struct Command2 {
            DrawCommand cmd;
            // This is float4.
            alignas(16) math::Rect clip;
            u32 data[8];
        };
        static_assert(sizeof(Command2) == 64);

        void pushCommand(DrawCommand cmd) {
            commands_.push(base::toUnderlying(cmd));
        }
        void pushDatum(u32 x) { data_.push(x); }
        void pushDatum(float x) {
            data_.push(base::bit_cast<float, u32>(x * scale_));
        }
        template <typename... Ts>
        void pushData(Ts... ts) {
            (pushDatum(ts), ...);
        }

        void sampleTexture(float x, float y, float width, float height,
                           UIRenderer::TextureHandle handle, u32 color);

        template <typename... Ts>
        void pushCommand2(DrawCommand cmd, Ts... ts) {
            commands2_.push(
                Command2{cmd, clip_, {base::bit_cast<Ts, u32>(ts)...}});
        }

        math::Vector2f currentTransform() const {
            return transform_stack_[transform_stack_.count() - 1];
        }

        float scale_ = 1.0;
        UIRenderer* renderer_ = nullptr;
        base::Vector<u8> commands_;
        base::Vector<u32> data_;

        math::Rect clip_;
        base::Vector<Command2> commands2_;

        base::Vector<math::Vector2f> transform_stack_;

        usize estimated_slots_ = 0;
    };
}  // namespace spargel::render
