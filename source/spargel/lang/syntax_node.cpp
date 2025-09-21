#include "spargel/lang/syntax_node.h"

#include "spargel/base/check.h"
#include "spargel/base/console.h"

namespace spargel::lang {
    void SyntaxNode::dump(usize indent) const {
        auto& console = base::Console::instance();
        console.write_n(' ', indent);
        console.write("[node] ");
        console.write(toStringLiteral(kind_));
        console.write('\n');
        for (auto const& child : children_) {
            child.dump(indent + 4);
        }
    }
    void SyntaxChild::dump(usize indent) const {
        if (isNode()) {
            asNode().dump(indent);
        } else {
            asToken().dump(indent);
        }
    }
    void SyntaxBuilder::pushToken(SyntaxToken token) {
        current().children_.emplace(token);
    }
    void SyntaxBuilder::beginNode(SyntaxKind kind) { stack_.emplace(kind); }
    void SyntaxBuilder::endNode() {
        spargel_check(stack_.count() >= 2);
        auto& parent = stack_[stack_.count() - 2];
        parent.children_.emplace(base::move(stack_[stack_.count() - 1]));
        stack_.pop();
    }
}  // namespace spargel::lang
