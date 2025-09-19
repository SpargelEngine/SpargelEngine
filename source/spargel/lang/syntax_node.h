#pragma once

#include "spargel/base/either.h"
#include "spargel/base/vector.h"
#include "spargel/lang/syntax_kind.h"
#include "spargel/lang/syntax_token.h"

namespace spargel::lang {
    class SyntaxChild;
    class SyntaxNode {
    public:
        explicit SyntaxNode(SyntaxKind kind) : kind_{kind} {}

        SyntaxKind kind() const { return kind_; }

        base::Span<SyntaxChild> children() const { return children_.toSpan(); }

        void dump(usize indent = 0) const;

    private:
        friend class SyntaxBuilder;

        SyntaxKind kind_;
        base::Vector<SyntaxChild> children_;
    };
    class SyntaxChild {
    public:
        explicit SyntaxChild(SyntaxToken token) : value_{base::Left{token}} {}
        explicit SyntaxChild(SyntaxKind kind) : value_{base::Right{SyntaxNode{kind}}} {}
        explicit SyntaxChild(SyntaxNode&& node) : value_{base::Right{base::move(node)}} {}

        bool isToken() const { return value_.isLeft(); }
        bool isNode() const { return value_.isRight(); }

        SyntaxToken& asToken() { return value_.left(); }
        SyntaxToken const& asToken() const { return value_.left(); }
        SyntaxNode& asNode() { return value_.right(); }
        SyntaxNode const& asNode() const { return value_.right(); }

        void dump(usize indent) const;

    private:
        base::Either<SyntaxToken, SyntaxNode> value_;
    };
    class SyntaxBuilder {
    public:
        void unsafeEmplace(SyntaxKind kind) { stack_.emplace(kind); }

        // Start building a new child node.
        void beginNode(SyntaxKind kind);
        // Finish the current child.
        void endNode();
        void pushToken(SyntaxToken token);

        SyntaxNode& current() {
            spargel_check(!empty());
            return stack_[stack_.count() - 1];
        }

        bool empty() const { return stack_.empty(); }

        SyntaxNode result() {
            spargel_check(stack_.count() == 1);
            auto ret = base::move(stack_[0]);
            stack_.clear();
            return ret;
        }

    private:
        base::Vector<SyntaxNode> stack_;
    };
}  // namespace spargel::lang
