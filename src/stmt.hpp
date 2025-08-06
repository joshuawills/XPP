#ifndef STMT_HPP
#define STMT_HPP

#include "./ast.hpp"
#include "./visitor.hpp"

class LocalVarDecl;
class Expr;

class Stmt : public AST {
 public:
    Stmt(Position pos)
    : AST(pos) {}

    virtual ~Stmt() = default;
    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;
    auto print(std::ostream& os) const -> void override = 0;

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
    auto print(std::ostream& os) const -> void override;
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
    auto print(std::ostream& os) const -> void override;

    auto get_decl() const -> std::shared_ptr<LocalVarDecl> {
        return decl_;
    }

 private:
    std::shared_ptr<LocalVarDecl> decl_;
};

class ReturnStmt
: public Stmt
, public std::enable_shared_from_this<ReturnStmt> {
 public:
    ReturnStmt(Position const pos, std::shared_ptr<Expr> const expr)
    : Stmt(pos)
    , expr_(expr) {}

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_return_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const expr_;
};

class ExprStmt
: public Stmt
, public std::enable_shared_from_this<ExprStmt> {
 public:
    ExprStmt(Position const pos, std::shared_ptr<Expr> const expr)
    : Stmt(pos)
    , expr_(expr) {}

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_expr_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const expr_;
};

class WhileStmt
: public Stmt
, public std::enable_shared_from_this<WhileStmt> {
 public:
    WhileStmt(Position const pos, std::shared_ptr<Expr> const cond, std::vector<std::shared_ptr<Stmt>> const stmts)
    : Stmt(pos)
    , cond_(cond)
    , stmts_{stmts} {}

    auto get_cond() const -> std::shared_ptr<Expr> {
        return cond_;
    }

    auto get_stmts() const -> std::vector<std::shared_ptr<Stmt>> {
        return stmts_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_while_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const cond_;
    std::vector<std::shared_ptr<Stmt>> stmts_;
};

#endif // STMT_HPP