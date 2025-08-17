#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <memory>

class ParaDecl;
class LocalVarDecl;
class GlobalVarDecl;
class EnumDecl;
class ClassFieldDecl;
class ClassDecl;
class Function;
class MethodDecl;
class ConstructorDecl;
class EmptyExpr;
class AssignmentExpr;
class BinaryExpr;
class UnaryExpr;
class DecimalExpr;
class IntExpr;
class UIntExpr;
class BoolExpr;
class StringExpr;
class CharExpr;
class VarExpr;
class CallExpr;
class CastExpr;
class ArrayInitExpr;
class ArrayIndexExpr;
class EnumAccessExpr;
class EmptyStmt;
class CompoundStmt;
class LocalVarStmt;
class ReturnStmt;
class ExprStmt;
class WhileStmt;
class IfStmt;
class ElseIfStmt;
class Extern;

class Visitor {
 public:
    virtual auto visit_para_decl(std::shared_ptr<ParaDecl> para_decl) -> void = 0;
    virtual auto visit_local_var_decl(std::shared_ptr<LocalVarDecl> local_var_decl) -> void = 0;
    virtual auto visit_global_var_decl(std::shared_ptr<GlobalVarDecl> global_var_decl) -> void = 0;
    virtual auto visit_enum_decl(std::shared_ptr<EnumDecl> enum_decl) -> void = 0;
    virtual auto visit_class_decl(std::shared_ptr<ClassDecl> class_decl) -> void = 0;
    virtual auto visit_class_field_decl(std::shared_ptr<ClassFieldDecl> class_field_decl) -> void = 0;
    virtual auto visit_function(std::shared_ptr<Function> function) -> void = 0;
    virtual auto visit_method_decl(std::shared_ptr<MethodDecl> method_decl) -> void = 0;
    virtual auto visit_constructor_decl(std::shared_ptr<ConstructorDecl> constructor_decl) -> void = 0;
    virtual auto visit_extern(std::shared_ptr<Extern> extern_) -> void = 0;
    virtual auto visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void = 0;
    virtual auto visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void = 0;
    virtual auto visit_binary_expr(std::shared_ptr<BinaryExpr> binary_expr) -> void = 0;
    virtual auto visit_unary_expr(std::shared_ptr<UnaryExpr> unary_expr) -> void = 0;
    virtual auto visit_decimal_expr(std::shared_ptr<DecimalExpr> decimal_expr) -> void = 0;
    virtual auto visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void = 0;
    virtual auto visit_uint_expr(std::shared_ptr<UIntExpr> uint_expr) -> void = 0;
    virtual auto visit_bool_expr(std::shared_ptr<BoolExpr> bool_expr) -> void = 0;
    virtual auto visit_string_expr(std::shared_ptr<StringExpr> string_expr) -> void = 0;
    virtual auto visit_char_expr(std::shared_ptr<CharExpr> char_expr) -> void = 0;
    virtual auto visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void = 0;
    virtual auto visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void = 0;
    virtual auto visit_cast_expr(std::shared_ptr<CastExpr> cast_expr) -> void = 0;
    virtual auto visit_array_init_expr(std::shared_ptr<ArrayInitExpr> array_init_expr) -> void = 0;
    virtual auto visit_array_index_expr(std::shared_ptr<ArrayIndexExpr> array_index_expr) -> void = 0;
    virtual auto visit_enum_access_expr(std::shared_ptr<EnumAccessExpr> enum_access_expr) -> void = 0;

    virtual auto visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void = 0;
    virtual auto visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void = 0;
    virtual auto visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void = 0;
    virtual auto visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void = 0;
    virtual auto visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void = 0;
    virtual auto visit_while_stmt(std::shared_ptr<WhileStmt> while_stmt) -> void = 0;
    virtual auto visit_if_stmt(std::shared_ptr<IfStmt> if_stmt) -> void = 0;
    virtual auto visit_else_if_stmt(std::shared_ptr<ElseIfStmt> else_if_stmt) -> void = 0;
};

#endif // VISITOR_HPP