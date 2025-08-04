#ifndef EXPR_HPP
#define EXPR_HPP

#include "./ast.hpp"
#include "./type.hpp"
#include "./visitor.hpp"

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
};

class Expr : public AST {
 public:
    Expr(Position pos, Type t)
    : AST(pos)
    , t_(t) {}

    auto get_type() const -> Type const& {
        return t_;
    }

    virtual ~Expr() = default;
    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;

 private:
    Type t_;
};

class EmptyExpr : public Expr {
 public:
    EmptyExpr(Position pos)
    : Expr(pos, Type{TypeSpec::UNKNOWN, std::nullopt}) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_empty_expr(std::make_shared<EmptyExpr>(*this));
    }

 private:
};

class AssignmentExpr : public Expr {
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
        visitor->visit_assignment_expr(std::make_shared<AssignmentExpr>(*this));
    }

 private:
    std::shared_ptr<Expr> left_;
    Operator op_;
    std::shared_ptr<Expr> right_;
};

class BinaryExpr : public Expr {
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
        visitor->visit_binary_expr(std::make_shared<BinaryExpr>(*this));
    }

 private:
    std::shared_ptr<Expr> left_;
    Operator op_;
    std::shared_ptr<Expr> right_;
};

class UnaryExpr : public Expr {
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
        visitor->visit_unary_expr(std::make_shared<UnaryExpr>(*this));
    }

 private:
    Operator op_;
    std::shared_ptr<Expr> expr_;
};

class IntExpr : public Expr {
 public:
    IntExpr(Position pos, int64_t value)
    : Expr(pos, Type{TypeSpec::I64, std::nullopt})
    , value_(value) {}

    auto get_value() const -> int64_t {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_int_expr(std::make_shared<IntExpr>(*this));
    }

 private:
    int64_t value_;
};

class VarExpr : public Expr {
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
        visitor->visit_var_expr(std::make_shared<VarExpr>(*this));
    }

 private:
    std::string name_;
};

#endif // EXPR_HPP