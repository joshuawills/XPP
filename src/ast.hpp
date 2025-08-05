#ifndef AST_HPP
#define AST_HPP

#include "./emitter.hpp"
#include "./token.hpp"
#include "./visitor.hpp"
#include <string>

#include "llvm/IR/Value.h"

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

    virtual auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* = 0;
    virtual auto print(std::ostream& os) const -> void = 0;

    auto pos() const -> Position {
        return pos_;
    }

    virtual auto visit(std::shared_ptr<Visitor> visitor) -> void = 0;

 private:
    const Position pos_;
    AST* parent_ = nullptr;
};

#endif // AST_HPP