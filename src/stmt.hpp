#ifndef STMT_HPP
#define STMT_HPP

#include "./ast.hpp"

class LocalVarDecl;

class Stmt : public AST {
 public:
    Stmt(Position pos)
    : AST(pos) {}

 private:
};

class EmptyStmt : public Stmt {
 public:
    EmptyStmt(Position pos)
    : Stmt(pos) {}
};

class LocalVarStmt : public Stmt {
 public:
    LocalVarStmt(Position pos, std::shared_ptr<LocalVarDecl> decl)
    : Stmt(pos)
    , decl_(decl) {}

 private:
    std::shared_ptr<LocalVarDecl> decl_;
};

#endif // STMT_HPP