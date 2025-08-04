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

 private:
    Type t_;
};

class EmptyExpr
: public Expr
, public std::enable_shared_from_this<EmptyExpr> {
 public:
    EmptyExpr(Position pos)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt}) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_empty_expr(shared_from_this());
    }

 private:
};

class AssignmentExpr
: public Expr
, public std::enable_shared_from_this<AssignmentExpr> {
 public:
    AssignmentExpr(Position pos, std::shared_ptr<Expr> left, Operator op, std::shared_ptr<Expr> right)
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

 private:
    std::shared_ptr<Expr> left_;
    Operator op_;
    std::shared_ptr<Expr> right_;
};

class BinaryExpr
: public Expr
, public std::enable_shared_from_this<BinaryExpr> {
 public:
    BinaryExpr(Position pos, std::shared_ptr<Expr> left, Operator op, std::shared_ptr<Expr> right)
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

 private:
    std::shared_ptr<Expr> left_;
    Operator op_;
    std::shared_ptr<Expr> right_;
};

class UnaryExpr
: public Expr
, public std::enable_shared_from_this<UnaryExpr> {
 public:
    UnaryExpr(Position pos, Operator op, std::shared_ptr<Expr> expr)
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

 private:
    Operator op_;
    std::shared_ptr<Expr> expr_;
};

class IntExpr
: public Expr
, public std::enable_shared_from_this<IntExpr> {
 public:
    IntExpr(Position pos, int64_t value)
    : Expr(pos, Type{TypeSpec::I64, std::nullopt})
    , value_(value) {}

    auto get_value() const -> int64_t {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_int_expr(shared_from_this());
    }

 private:
    int64_t value_;
};

class BoolExpr
: public Expr
, public std::enable_shared_from_this<BoolExpr> {
 public:
    BoolExpr(Position pos, bool value)
    : Expr(pos, Type{TypeSpec::BOOL, std::nullopt})
    , value_(value) {}

    auto get_value() const -> bool {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_bool_expr(shared_from_this());
    }

 private:
    bool value_;
};

class VarExpr
: public Expr
, public std::enable_shared_from_this<VarExpr> {
 public:
    VarExpr(Position pos, std::string name, Type t)
    : Expr(pos, t)
    , name_(name) {}

    VarExpr(Position pos, std::string name)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt})
    , name_(name) {}

    auto get_name() const -> std::string const& {
        return name_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_var_expr(shared_from_this());
    }

    auto set_ref(std::shared_ptr<Decl> ref) -> void {
        ref_ = ref;
    }

    auto get_ref() const -> std::shared_ptr<Decl> {
        return ref_;
    }

 private:
    std::string name_;
    std::shared_ptr<Decl> ref_ = nullptr;
};

#endif // EXPR_HPP