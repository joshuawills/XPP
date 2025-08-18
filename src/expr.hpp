#ifndef EXPR_HPP
#define EXPR_HPP

#include "./ast.hpp"
#include "./type.hpp"
#include "./visitor.hpp"

#include <iostream>

class Decl;

enum Op {
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
    DEREF,
    ADDRESS_OF,
    PREFIX_ADD,
    PREFIX_MINUS,
    POSTFIX_ADD,
    POSTFIX_MINUS,
    MODULO,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MULTIPLY_ASSIGN,
    DIVIDE_ASSIGN
};

auto operator<<(std::ostream& os, Op const& o) -> std::ostream&;

class Expr : public AST {
 public:
    Expr(Position pos, std::shared_ptr<Type> t)
    : AST(pos)
    , t_(t) {}

    auto get_type() const -> std::shared_ptr<Type> const& {
        return t_;
    }

    auto set_type(std::shared_ptr<Type> t) -> void {
        t_ = t;
    }

    virtual ~Expr() = default;
    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;
    auto print(std::ostream& os) const -> void override = 0;

 private:
    std::shared_ptr<Type> t_;
};

class EmptyExpr
: public Expr
, public std::enable_shared_from_this<EmptyExpr> {
 public:
    EmptyExpr(Position pos)
    : Expr(pos, std::make_shared<Type>(TypeSpec::VOID)) {}

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
    AssignmentExpr(Position pos, std::shared_ptr<Expr> left, Op const op, std::shared_ptr<Expr> right)
    : Expr(pos, std::make_shared<Type>())
    , left_(left)
    , op_(op)
    , right_(right) {}

    auto get_left() const -> std::shared_ptr<Expr> {
        return left_;
    }
    auto get_right() const -> std::shared_ptr<Expr> {
        return right_;
    }
    auto get_operator() const -> Op {
        return op_;
    }

    auto set_rhs_expression(std::shared_ptr<Expr> expr) -> void {
        right_ = expr;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_assignment_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> left_;
    Op const op_;
    std::shared_ptr<Expr> right_;
};

class BinaryExpr
: public Expr
, public std::enable_shared_from_this<BinaryExpr> {
 public:
    BinaryExpr(Position const pos, std::shared_ptr<Expr> const left, Op const op, std::shared_ptr<Expr> const right)
    : Expr(pos, std::make_shared<Type>())
    , left_(left)
    , op_(op)
    , right_(right) {}

    auto get_left() const -> std::shared_ptr<Expr> {
        return left_;
    }
    auto get_right() const -> std::shared_ptr<Expr> {
        return right_;
    }
    auto get_operator() const -> Op {
        return op_;
    }

    auto set_pointer_arithmetic() -> void {
        is_pointer_arithmetic_ = true;
    }

    auto is_pointer_arithmetic() -> bool {
        return is_pointer_arithmetic_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_binary_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<Expr> const left_;
    Op const op_;
    std::shared_ptr<Expr> const right_;
    bool is_pointer_arithmetic_ = false;

    auto handle_logical_or(std::shared_ptr<Emitter> emitter) -> llvm::Value*;
    auto handle_logical_and(std::shared_ptr<Emitter> emitter) -> llvm::Value*;
};

class UnaryExpr
: public Expr
, public std::enable_shared_from_this<UnaryExpr> {
 public:
    UnaryExpr(Position const pos, Op const op, std::shared_ptr<Expr> expr)
    : Expr(pos, std::make_shared<Type>())
    , op_(op)
    , expr_(expr) {}

    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }
    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }
    auto get_operator() const -> Op {
        return op_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_unary_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    Op const op_;
    std::shared_ptr<Expr> expr_;
};

class IntExpr
: public Expr
, public std::enable_shared_from_this<IntExpr> {
 public:
    IntExpr(Position const pos, int64_t value)
    : Expr(pos, std::make_shared<Type>(TypeSpec::I64))
    , value_(value) {}

    auto get_value() const -> int64_t {
        return value_;
    }

    auto set_width(uint8_t width) -> void {
        width_ = width;
    }

    auto get_width() -> uint8_t {
        return width_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_int_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    int64_t const value_;
    uint8_t width_ = 64;
};

class UIntExpr
: public Expr
, public std::enable_shared_from_this<UIntExpr> {
 public:
    UIntExpr(Position const pos, uint64_t value)
    : Expr(pos, std::make_shared<Type>(TypeSpec::U64))
    , value_(value) {}

    auto get_value() const -> uint64_t {
        return value_;
    }

    auto set_width(uint8_t width) -> void {
        width_ = width;
    }

    auto get_width() -> uint8_t {
        return width_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_uint_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    uint64_t const value_;
    uint8_t width_ = 64;
};

class DecimalExpr
: public Expr
, public std::enable_shared_from_this<DecimalExpr> {
 public:
    DecimalExpr(Position const pos, double value)
    : Expr(pos, std::make_shared<Type>(TypeSpec::F64))
    , value_(value) {}

    auto get_value() const -> double {
        return value_;
    }

    auto set_width(uint8_t width) -> void {
        width_ = width;
    }

    auto get_width() -> uint8_t {
        return width_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_decimal_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    double const value_;
    uint8_t width_ = 64;
};

class BoolExpr
: public Expr
, public std::enable_shared_from_this<BoolExpr> {
 public:
    BoolExpr(Position const pos, bool value)
    : Expr(pos, std::make_shared<Type>(TypeSpec::BOOL))
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

class StringExpr
: public Expr
, public std::enable_shared_from_this<StringExpr> {
 public:
    StringExpr(Position const pos, std::string value)
    : Expr(pos, std::make_shared<PointerType>(std::make_shared<Type>(TypeSpec::I8)))
    , value_(value) {}

    auto get_value() const -> std::string {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_string_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::string const value_;
};

class CharExpr
: public Expr
, public std::enable_shared_from_this<CharExpr> {
 public:
    CharExpr(Position const pos, char const value)
    : Expr(pos, std::make_shared<Type>(TypeSpec::I8))
    , value_(value) {}

    auto get_value() const -> char {
        return value_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_char_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    char const value_;
};

class VarExpr
: public Expr
, public std::enable_shared_from_this<VarExpr> {
 public:
    VarExpr(Position pos, std::string name, std::shared_ptr<Type> t)
    : Expr(pos, t)
    , name_(name) {}

    VarExpr(Position const pos, std::string const name)
    : Expr(pos, std::make_shared<Type>())
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

    auto get_ref() -> std::shared_ptr<Decl> {
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
    : Expr(pos, std::make_shared<Type>())
    , name_(name)
    , args_(args) {}

    CallExpr(Position const pos,
             std::string const name,
             std::shared_ptr<Type> t,
             std::vector<std::shared_ptr<Expr>> const args)
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

    auto set_ref(std::shared_ptr<Decl> ref) -> void {
        ref_ = ref;
    }

    auto get_ref() const -> std::shared_ptr<Decl> {
        return ref_;
    }

    auto get_args() -> std::vector<std::shared_ptr<Expr>> {
        return args_;
    }

 private:
    std::string const name_;
    std::vector<std::shared_ptr<Expr>> const args_;
    std::shared_ptr<Decl> ref_ = nullptr;
};

class ConstructorCallExpr
: public Expr
, public std::enable_shared_from_this<ConstructorCallExpr> {
 public:
    ConstructorCallExpr(Position const pos, std::string const name, std::vector<std::shared_ptr<Expr>> const args)
    : Expr(pos, std::make_shared<Type>())
    , name_(name)
    , args_(args) {}

    ConstructorCallExpr(Position const pos,
                        std::shared_ptr<Type> t,
                        std::string const name,
                        std::vector<std::shared_ptr<Expr>> const args)
    : Expr(pos, t)
    , name_(name)
    , args_(args) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_constructor_call_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_name() const -> std::string {
        return name_;
    }

    auto set_ref(std::shared_ptr<Decl> ref) -> void {
        ref_ = ref;
    }

    auto get_ref() const -> std::shared_ptr<Decl> {
        return ref_;
    }

    auto get_args() -> std::vector<std::shared_ptr<Expr>> {
        return args_;
    }

 private:
    std::string const name_;
    std::vector<std::shared_ptr<Expr>> const args_;
    std::shared_ptr<Decl> ref_ = nullptr;
};

class CastExpr
: public Expr
, public std::enable_shared_from_this<CastExpr> {
 public:
    CastExpr(Position const pos, std::shared_ptr<Expr> expr, std::shared_ptr<Type> const to)
    : Expr(pos, to)
    , expr_(expr)
    , to_(to) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_cast_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }
    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

    auto get_to_type() const -> std::shared_ptr<Type> {
        return to_;
    }

 private:
    std::shared_ptr<Expr> expr_;
    std::shared_ptr<Type> const to_;
};

class ArrayInitExpr
: public Expr
, public std::enable_shared_from_this<ArrayInitExpr> {
 public:
    ArrayInitExpr(Position const pos, std::vector<std::shared_ptr<Expr>> exprs)
    : Expr(pos, std::make_shared<Type>())
    , exprs_(exprs) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_array_init_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto set_exprs(std::vector<std::shared_ptr<Expr>> exprs) -> void {
        exprs_ = std::move(exprs);
    }

    auto get_exprs() const -> std::vector<std::shared_ptr<Expr>> {
        return exprs_;
    }

 private:
    std::vector<std::shared_ptr<Expr>> exprs_;
};

class ArrayIndexExpr
: public Expr
, public std::enable_shared_from_this<ArrayIndexExpr> {
 public:
    ArrayIndexExpr(Position const pos, std::shared_ptr<Expr> array_expr, std::shared_ptr<Expr> index_expr)
    : Expr(pos, std::make_shared<Type>())
    , array_expr_(array_expr)
    , index_expr_(index_expr) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_array_index_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_array_expr() const -> std::shared_ptr<Expr> {
        return array_expr_;
    }

    auto get_index_expr() const -> std::shared_ptr<Expr> {
        return index_expr_;
    }

    auto set_index_expr(std::shared_ptr<Expr> e) -> void {
        index_expr_ = e;
    }

 private:
    std::shared_ptr<Expr> array_expr_, index_expr_;
};

class EnumAccessExpr
: public Expr
, public std::enable_shared_from_this<EnumAccessExpr> {
 public:
    EnumAccessExpr(Position const pos, std::string const& enum_name, std::string const& field)
    : Expr(pos, std::make_shared<Type>())
    , enum_name_(enum_name)
    , field_(field) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_enum_access_expr(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_enum_name() const -> std::string {
        return enum_name_;
    }

    auto get_field() const -> std::string {
        return field_;
    }

    auto set_field_num(int field_num) -> void {
        field_num_ = field_num;
    }

    auto get_field_num() -> int {
        return field_num_;
    }

 private:
    std::string const enum_name_;
    std::string const field_;
    int field_num_;
};

#endif // EXPR_HPP
