#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <memory>

class ParaDecl;
class LocalVarDecl;
class Function;
class EmptyExpr;
class AssignmentExpr;
class BinaryExpr;
class UnaryExpr;
class IntExpr;
class BoolExpr;
class StringExpr;
class VarExpr;
class CallExpr;
class EmptyStmt;
class LocalVarStmt;
class ReturnStmt;
class ExprStmt;
class Extern;

class Visitor {
 public:
    virtual auto visit_para_decl(std::shared_ptr<ParaDecl> para_decl) -> void = 0;
    virtual auto visit_local_var_decl(std::shared_ptr<LocalVarDecl> local_var_decl) -> void = 0;
    virtual auto visit_function(std::shared_ptr<Function> function) -> void = 0;
    virtual auto visit_extern(std::shared_ptr<Extern> extern_) -> void = 0;
    virtual auto visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void = 0;
    virtual auto visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void = 0;
    virtual auto visit_binary_expr(std::shared_ptr<BinaryExpr> binary_expr) -> void = 0;
    virtual auto visit_unary_expr(std::shared_ptr<UnaryExpr> unary_expr) -> void = 0;
    virtual auto visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void = 0;
    virtual auto visit_bool_expr(std::shared_ptr<BoolExpr> bool_expr) -> void = 0;
    virtual auto visit_string_expr(std::shared_ptr<StringExpr> string_expr) -> void = 0;
    virtual auto visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void = 0;
    virtual auto visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void = 0;

    virtual auto visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void = 0;
    virtual auto visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void = 0;
    virtual auto visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void = 0;
    virtual auto visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void = 0;
};

#endif // VISITOR_HPP