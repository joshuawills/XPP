#ifndef AST_HPP
#define AST_HPP

#include <string>
#include "./token.hpp"

class AST {
    public:
        AST(Position pos):
            pos_(pos) {}

        AST(Position pos,  AST *parent):
            pos_(pos), parent_(parent) {}

        auto set_parent(AST* parent) {
            parent_ = parent;
        }

        virtual ~AST() = default;
    private:
        Position pos_;
        AST* parent_ = nullptr;
};

#endif // AST_HPP