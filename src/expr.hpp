#ifndef EXPR_HPP
#define EXPR_HPP

#include "./ast.hpp"
#include "./type.hpp"
#include "./visitor.hpp"

#include <iostream>

class Decl;

enum Operator {
    ASSIGN,
    LOGICAL_OR,
    LOGICAL_AND,
    EQUAL,
    NOT_EQUAL,
    NEGATE,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
};

auto operator<<(std::ostream& os, Operator const& o) -> std::ostream&;

class Expr : public AST {
 public:
    Expr(Position pos, Type t)
    : AST(pos)
    , t_(t) {}

    auto get_type() const -> Type const& {
        return t_;
    }

    auto set_type(Type t) -> void {
        t_ = t;
    }

    virtual ~Expr() = default;
    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;
    auto print(std::ostream& os) const -> void override = 0;

 private:
    Type t_;
};

class EmptyExpr
: public Expr
, public std::enable_shared_from_this<EmptyExpr> {
 public:
    EmptyExpr(Position pos)
    : Expr(pos, Type{TypeSpec::VOID, std::nullopt}) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_empty_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;
};

class AssignmentExpr
: public Expr
, public std::enable_shared_from_this<AssignmentExpr> {
 public:
    AssignmentExpr(Position pos, std::shared_ptr<Expr> const left, Operator const op, std::shared_ptr<Expr> const right)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , left_(left)
    , op_(op)
    , right_(right) {}

    auto get_left() const -> std::shared_ptr<Expr> {
        return left_;
    }
    auto get_right() const -> std::shared_ptr<Expr> {
        return right_;
    }
    auto get_operator() const -> Operator {
        return op_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_assignment_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const left_;
    Operator const op_;
    std::shared_ptr<Expr> const right_;
};

class BinaryExpr
: public Expr
, public std::enable_shared_from_this<BinaryExpr> {
 public:
    BinaryExpr(Position const pos, std::shared_ptr<Expr> const left, Operator const op, std::shared_ptr<Expr> const right)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , left_(left)
    , op_(op)
    , right_(right) {}

    auto get_left() const -> std::shared_ptr<Expr> {
        return left_;
    }
    auto get_right() const -> std::shared_ptr<Expr> {
        return right_;
    }
    auto get_operator() const -> Operator {
        return op_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_binary_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const left_;
    Operator const op_;
    std::shared_ptr<Expr> const right_;

    auto handle_logical_or(std::shared_ptr<Emitter> emitter) -> llvm::Value*;
    auto handle_logical_and(std::shared_ptr<Emitter> emitter) -> llvm::Value*;
};

class UnaryExpr
: public Expr
, public std::enable_shared_from_this<UnaryExpr> {
 public:
    UnaryExpr(Position const pos, Operator const op, std::shared_ptr<Expr> const expr)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , op_(op)
    , expr_(expr) {}

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }
    auto get_operator() const -> Operator {
        return op_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_unary_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    Operator const op_;
    std::shared_ptr<Expr> const expr_;
};

class IntExpr
: public Expr
, public std::enable_shared_from_this<IntExpr> {
 public:
    IntExpr(Position const pos, int64_t value)
    : Expr(pos, Type{TypeSpec::I64, std::nullopt})
    , value_(value) {}

    auto get_value() const -> int64_t {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_int_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    int64_t const value_;
};

class BoolExpr
: public Expr
, public std::enable_shared_from_this<BoolExpr> {
 public:
    BoolExpr(Position const pos, bool value)
    : Expr(pos, Type{TypeSpec::BOOL, std::nullopt})
    , value_(value) {}

    auto get_value() const -> bool {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_bool_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    bool const value_;
};

class VarExpr
: public Expr
, public std::enable_shared_from_this<VarExpr> {
 public:
    VarExpr(Position pos, std::string name, Type t)
    : Expr(pos, t)
    , name_(name) {}

    VarExpr(Position const pos, std::string const name)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , name_(name) {}

    auto get_name() const -> std::string const& {
        return name_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_var_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto set_ref(std::shared_ptr<Decl> ref) -> void {
        ref_ = ref;
    }

    auto get_ref() const -> std::shared_ptr<Decl> {
        return ref_;
    }

 private:
    std::string const name_;
    std::shared_ptr<Decl> ref_ = nullptr;
};

class CallExpr
: public Expr
, public std::enable_shared_from_this<CallExpr> {
 public:
    CallExpr(Position const pos, std::string const name, std::vector<std::shared_ptr<Expr>> const args)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , name_(name)
    , args_(args) {}

    CallExpr(Position const pos, std::string const name, Type t, std::vector<std::shared_ptr<Expr>> const args)
    : Expr(pos, t)
    , name_(name)
    , args_(args) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_call_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_name() const -> std::string {
        return name_;
    }

    auto set_ref(std::shared_ptr<Function> ref) -> void {
        ref_ = ref;
    }

    auto get_ref() const -> std::shared_ptr<Function> {
        return ref_;
    }

    auto get_args() -> std::vector<std::shared_ptr<Expr>> {
        return args_;
    }

 private:
    std::string const name_;
    std::vector<std::shared_ptr<Expr>> const args_;
    std::shared_ptr<Function> ref_ = nullptr;
};

#endif // EXPR_HPP