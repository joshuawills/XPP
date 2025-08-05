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
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;

 private:
};

class EmptyStmt
: public Stmt
, public std::enable_shared_from_this<EmptyStmt> {
 public:
    EmptyStmt(Position pos)
    : Stmt(pos) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_empty_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
};

class LocalVarStmt
: public Stmt
, public std::enable_shared_from_this<LocalVarStmt> {
 public:
    LocalVarStmt(Position pos, std::shared_ptr<LocalVarDecl> decl)
    : Stmt(pos)
    , decl_(decl) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_local_var_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;

    auto get_decl() const -> std::shared_ptr<LocalVarDecl> {
        return decl_;
    }

 private:
    std::shared_ptr<LocalVarDecl> decl_;
};

#endif // STMT_HPP