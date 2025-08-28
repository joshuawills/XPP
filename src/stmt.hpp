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
    CompoundStmt(Position const pos, std::vector<std::shared_ptr<Stmt>> stmts)
    : Stmt(pos)
    , stmts_(stmts) {
        for (auto const& stmt : stmts_) {
            if (auto l = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
                has_return_ = true;
                break;
            }
        }
    }
    CompoundStmt(Position const pos)
    : Stmt(pos)
    , stmts_({}) {}

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

    auto add_stmt(std::shared_ptr<Stmt> stmt) -> void {
        stmts_.push_back(stmt);
    }

 private:
    std::vector<std::shared_ptr<Stmt>> stmts_;
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
    ReturnStmt(Position const pos, std::shared_ptr<Expr> expr)
    : Stmt(pos)
    , expr_(expr) {}

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }
    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_return_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> expr_;
};

class ExprStmt
: public Stmt
, public std::enable_shared_from_this<ExprStmt> {
 public:
    ExprStmt(Position const pos, std::shared_ptr<Expr> expr)
    : Stmt(pos)
    , expr_(expr) {}

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }
    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_expr_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> expr_;
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

class LoopStmt
: public Stmt
, public std::enable_shared_from_this<LoopStmt> {
 public:
    LoopStmt(Position const pos,
             std::string const& var_name,
             std::optional<std::shared_ptr<Expr>> lower_bound,
             std::optional<std::shared_ptr<Expr>> upper_bound,
             std::shared_ptr<Stmt> body_stmt)
    : Stmt(pos)
    , var_name_(var_name)
    , lower_bound_(lower_bound)
    , upper_bound_(upper_bound)
    , body_stmt_(body_stmt) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_loop_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto has_lower_bound() const -> bool {
        return lower_bound_.has_value();
    }
    auto get_lower_bound() const -> std::optional<std::shared_ptr<Expr>> {
        return lower_bound_;
    }
    auto set_lower_bound(std::shared_ptr<Expr> expr) -> void {
        lower_bound_ = expr;
    }
    auto has_upper_bound() const -> bool {
        return upper_bound_.has_value();
    }
    auto get_upper_bound() const -> std::optional<std::shared_ptr<Expr>> {
        return upper_bound_;
    }
    auto set_upper_bound(std::shared_ptr<Expr> expr) -> void {
        upper_bound_ = expr;
    }
    auto get_var_name() const -> std::string {
        return var_name_;
    }
    auto set_var_decl(std::shared_ptr<LocalVarDecl> var_decl) -> void {
        var_decl_ = var_decl;
    }
    auto get_var_decl() const -> std::shared_ptr<LocalVarDecl> {
        return var_decl_;
    }
    auto get_body_stmt() const -> std::shared_ptr<Stmt> {
        return body_stmt_;
    }

 private:
    std::string var_name_;
    std::optional<std::shared_ptr<Expr>> lower_bound_;
    std::optional<std::shared_ptr<Expr>> upper_bound_;
    std::shared_ptr<Stmt> body_stmt_;
    std::shared_ptr<LocalVarDecl> var_decl_;
};

class BreakStmt
: public Stmt
, public std::enable_shared_from_this<BreakStmt> {
 public:
    BreakStmt(Position const pos)
    : Stmt(pos) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_break_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;
};

class ContinueStmt
: public Stmt
, public std::enable_shared_from_this<ContinueStmt> {
 public:
    ContinueStmt(Position const pos)
    : Stmt(pos) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_continue_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;
};

class DeleteStmt
: public Stmt
, public std::enable_shared_from_this<DeleteStmt> {
 public:
    DeleteStmt(Position const pos, std::shared_ptr<Expr> expr)
    : Stmt(pos)
    , expr_(std::move(expr)) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_delete_stmt(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_pointer(std::shared_ptr<Emitter> emitter, std::shared_ptr<VarExpr> e) -> llvm::Value*;

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }
    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }

 private:
    std::shared_ptr<Expr> expr_;
};

#endif // STMT_HPP