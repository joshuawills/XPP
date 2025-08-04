#ifndef AST_HPP
#define AST_HPP

#include "./token.hpp"
#include "./visitor.hpp"
#include <string>

class AST {
 public:
    AST(Position pos)
    : pos_(pos) {}

    AST(Position pos, AST* parent)
    : pos_(pos)
    , parent_(parent) {}

    auto set_parent(AST* parent) {
        parent_ = parent;
    }

    virtual ~AST() = default;

    auto pos() const -> Position {
        return pos_;
    }

    virtual auto visit(std::shared_ptr<Visitor> visitor) -> void = 0;

 private:
    Position pos_;
    AST* parent_ = nullptr;
};

#endif // AST_HPP