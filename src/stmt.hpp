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

class CompoundStmt
: public Stmt
, public std::enable_shared_from_this<CompoundStmt> {
 public:
    CompoundStmt(Position const pos, std::vector<std::shared_ptr<Stmt>> const stmts)
    : Stmt(pos)
    , stmts_(stmts) {
        for (auto const& stmt : stmts_) {
            if (auto l = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
                has_return_ = true;
                break;
            }
        }
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_compound_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_stmts() const -> std::vector<std::shared_ptr<Stmt>> {
        return stmts_;
    }

    auto has_return() const noexcept -> bool {
        return has_return_;
    }

 private:
    std::vector<std::shared_ptr<Stmt>> const stmts_;
    bool has_return_ = false;
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
    WhileStmt(Position const pos, std::shared_ptr<Expr> const cond, std::shared_ptr<CompoundStmt> const compound_stmt)
    : Stmt(pos)
    , cond_(cond)
    , compound_stmt_{compound_stmt} {}

    auto get_cond() const -> std::shared_ptr<Expr> {
        return cond_;
    }

    auto get_stmts() const -> std::shared_ptr<CompoundStmt> {
        return compound_stmt_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_while_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const cond_;
    std::shared_ptr<CompoundStmt> compound_stmt_;
};

class IfStmt
: public Stmt
, public std::enable_shared_from_this<IfStmt> {
 public:
    IfStmt(Position const pos,
           std::shared_ptr<Expr> const cond,
           std::shared_ptr<Stmt> const stmt_one,
           std::shared_ptr<Stmt> const stmt_two,
           std::shared_ptr<Stmt> const stmt_three)
    : Stmt(pos)
    , cond_(cond)
    , stmt_one_(stmt_one)
    , stmt_two_(stmt_two)
    , stmt_three_(stmt_three) {}

    auto get_cond() const -> std::shared_ptr<Expr> {
        return cond_;
    }

    auto get_body_stmt() const -> std::shared_ptr<Stmt> {
        return stmt_one_;
    }

    auto get_else_if_stmt() const -> std::shared_ptr<Stmt> {
        return stmt_two_;
    }

    auto get_else_stmt() const -> std::shared_ptr<Stmt> {
        return stmt_three_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_if_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const cond_;
    std::shared_ptr<Stmt> const stmt_one_, stmt_two_, stmt_three_;
};

class ElseIfStmt
: public Stmt
, public std::enable_shared_from_this<ElseIfStmt> {
 public:
    ElseIfStmt(Position const pos,
               std::shared_ptr<Expr> const cond,
               std::shared_ptr<Stmt> const stmt_one,
               std::shared_ptr<Stmt> const stmt_two)
    : Stmt(pos)
    , cond_(cond)
    , stmt_one_(stmt_one)
    , stmt_two_(stmt_two) {}

    auto get_cond() const -> std::shared_ptr<Expr> {
        return cond_;
    }

    auto get_body_stmt() const -> std::shared_ptr<Stmt> {
        return stmt_one_;
    }

    auto get_nested_else_if_stmt() const -> std::shared_ptr<Stmt> {
        return stmt_two_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_else_if_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const cond_;
    std::shared_ptr<Stmt> const stmt_one_, stmt_two_;
};

#endif // STMT_HPP