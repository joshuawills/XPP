#ifndef STMT_HPP
#define STMT_HPP

#include "./ast.hpp"
#include "./visitor.hpp"

class LocalVarDecl;

class Stmt : public AST {
 public:
    Stmt(Position pos)
    : AST(pos) {}

    virtual ~Stmt() = default;
    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;

 private:
};

class EmptyStmt : public Stmt {
 public:
    EmptyStmt(Position pos)
    : Stmt(pos) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_empty_stmt(std::make_shared<EmptyStmt>(*this));
    }
};

class LocalVarStmt : public Stmt {
 public:
    LocalVarStmt(Position pos, std::shared_ptr<LocalVarDecl> decl)
    : Stmt(pos)
    , decl_(decl) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_local_var_stmt(std::make_shared<LocalVarStmt>(*this));
    }

 private:
    std::shared_ptr<LocalVarDecl> decl_;
};

#endif // STMT_HPP