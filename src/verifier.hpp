#ifndef VERIFIER_HPP
#define VERIFIER_HPP

#include <list>
#include <memory>

#include "./handler.hpp"
#include "./module.hpp"
#include "./visitor.hpp"

struct TableEntry {
    std::string id;
    int level;
    std::shared_ptr<Decl> attr;
};

class SymbolTable {
 public:
    SymbolTable() = default;

    auto open_scope() -> void {
        ++level_;
    }
    auto close_scope() -> void {
        while (!entries_.empty() and entries_.back().level == level_) {
            entries_.pop_back();
        }
        --level_;
    }
    auto insert(std::string id, std::shared_ptr<Decl> attr) -> void {
        entries_.push_back(TableEntry{id, level_, attr});
    }
    auto retrieve_one_level(std::string const& id) -> std::optional<TableEntry>;
    auto remove(TableEntry const& entry) -> void {
        entries_.remove_if([&](const TableEntry& e) { return e.id == entry.id and e.level == entry.level; });
    }

    auto retrieve_latest_scope() -> std::vector<TableEntry>;

    auto retrieve(std::string const& id) -> std::optional<TableEntry>;

 private:
    std::list<TableEntry> entries_;
    int level_ = 1;
};

class Verifier
: public std::enable_shared_from_this<Verifier>
, public Visitor {
 public:
    Verifier(std::shared_ptr<Handler> handler, std::shared_ptr<AllModules> modules)
    : handler_(handler)
    , modules_(modules) {}

    auto visit_para_decl(std::shared_ptr<ParaDecl> para_decl) -> void override;
    auto visit_local_var_decl(std::shared_ptr<LocalVarDecl> local_var_decl) -> void override;
    auto visit_global_var_decl(std::shared_ptr<GlobalVarDecl> global_var_decl) -> void override;
    auto visit_enum_decl(std::shared_ptr<EnumDecl> enum_decl) -> void override;
    auto visit_class_decl(std::shared_ptr<ClassDecl> class_decl) -> void override;
    auto visit_class_field_decl(std::shared_ptr<ClassFieldDecl> class_field_decl) -> void override;
    auto visit_function(std::shared_ptr<Function> function) -> void override;
    auto visit_method_decl(std::shared_ptr<MethodDecl> method_decl) -> void override;
    auto visit_constructor_decl(std::shared_ptr<ConstructorDecl> constructor_decl) -> void override;
    auto visit_destructor_decl(std::shared_ptr<DestructorDecl> destructor_decl) -> void override;
    auto visit_extern(std::shared_ptr<Extern> extern_) -> void override;
    auto visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void override;
    auto visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void override;
    auto visit_binary_expr(std::shared_ptr<BinaryExpr> binary_expr) -> void override;
    auto visit_unary_expr(std::shared_ptr<UnaryExpr> unary_expr) -> void override;
    auto visit_decimal_expr(std::shared_ptr<DecimalExpr> decimal_expr) -> void override;
    auto visit_null_expr(std::shared_ptr<NullExpr> null_expr) -> void override;
    auto visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void override;
    auto visit_uint_expr(std::shared_ptr<UIntExpr> uint_expr) -> void override;
    auto visit_bool_expr(std::shared_ptr<BoolExpr> bool_expr) -> void override;
    auto visit_string_expr(std::shared_ptr<StringExpr> string_expr) -> void override;
    auto visit_char_expr(std::shared_ptr<CharExpr> char_expr) -> void override;
    auto visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void override;
    auto visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void override;
    auto visit_constructor_call_expr(std::shared_ptr<ConstructorCallExpr> constructor_call_expr) -> void override;
    auto visit_cast_expr(std::shared_ptr<CastExpr> cast_expr) -> void override;
    auto visit_array_init_expr(std::shared_ptr<ArrayInitExpr> array_init_expr) -> void override;
    auto visit_array_index_expr(std::shared_ptr<ArrayIndexExpr> array_index_expr) -> void override;
    auto visit_enum_access_expr(std::shared_ptr<EnumAccessExpr> enum_access_expr) -> void override;
    auto visit_field_access_expr(std::shared_ptr<FieldAccessExpr> field_access_expr) -> void override;
    auto visit_method_access_expr(std::shared_ptr<MethodAccessExpr> method_access_expr) -> void override;
    auto visit_size_of_expr(std::shared_ptr<SizeOfExpr> size_of_expr) -> void override;
    auto visit_import_expr(std::shared_ptr<ImportExpr> import_expr) -> void override;
    auto visit_new_expr(std::shared_ptr<NewExpr> new_expr) -> void override;

    auto visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void override;
    auto visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void override;
    auto visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void override;
    auto visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void override;
    auto visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void override;
    auto visit_while_stmt(std::shared_ptr<WhileStmt> while_stmt) -> void override;
    auto visit_if_stmt(std::shared_ptr<IfStmt> if_stmt) -> void override;
    auto visit_else_if_stmt(std::shared_ptr<ElseIfStmt> else_if_stmt) -> void override;
    auto visit_loop_stmt(std::shared_ptr<LoopStmt> loop_stmt) -> void override;
    auto visit_break_stmt(std::shared_ptr<BreakStmt> break_stmt) -> void override;
    auto visit_continue_stmt(std::shared_ptr<ContinueStmt> continue_stmt) -> void override;
    auto visit_delete_stmt(std::shared_ptr<DeleteStmt> delete_stmt) -> void override;

    auto check(std::string const& filename, bool is_main, bool is_libc = false) -> void;

    std::optional<std::shared_ptr<Type>> current_numerical_type = std::nullopt;
    Position unmurk_pos;
    std::shared_ptr<ClassDecl> curr_class = nullptr;
    std::shared_ptr<Module> curr_module_access_ = nullptr;
    std::string curr_module_alias_ = {};

    std::shared_ptr<Expr> updated_expr_ = nullptr;
    bool in_constructor_ = false;
    bool in_destructor_ = false;
    bool visiting_lhs_of_assignment_ = false;

 private:
    std::shared_ptr<Handler> handler_;
    std::shared_ptr<AllModules> modules_;

    SymbolTable symbol_table_ = {};

    bool has_main_ = false, has_return_ = false, in_main_ = false;

    size_t global_statement_counter_ = 0;
    int loop_depth_ = 0;

    std::string current_filename_;
    std::shared_ptr<Module> current_module_ = nullptr;
    std::shared_ptr<Decl> current_function_or_method_ = nullptr;

    std::vector<std::string> const all_errors_ = {"0: main function is missing",
                                                  "1: duplicate function declaration: %",
                                                  "2: invalid main function signature: %",
                                                  "3: identifier redeclared in the same scope: %",
                                                  "4: identifier declared void: %",
                                                  "5: incompatible type for this binary operator: %",
                                                  "6: incompatible type for this assignment: %",
                                                  "7: LHS of assignment must be a variable",
                                                  "8: variable not declared in this scope: %",
                                                  "9: incompatible type for this unary operator: %",
                                                  "10: missing return stmt: %",
                                                  "11: incompatible type for return: %",
                                                  "12: no such function with name: %",
                                                  "13: main function may not call itself",
                                                  "14: incorrect parameters for function: %",
                                                  "15: duplicate extern declaration: %",
                                                  "16: user functions can't utilise variatics: %",
                                                  "17: variatic type may only be last specified type in extern "
                                                  "declaration",
                                                  "18: character literal can only have one character in it",
                                                  "19: while stmt condition is not boolean: %",
                                                  "20: cannot mutate constant variable: %",
                                                  "21: unused variable: %",
                                                  "22: unused function: %",
                                                  "23: unused extern: %",
                                                  "24: if statement condition is not boolean: %",
                                                  "25: address-of operand can only be performed to allocated variables",
                                                  "26: can't get address of a constant variable: %",
                                                  "27: invalid type cast operation: %",
                                                  "28: prefix/postfix operators may only be applied to lvalue types",
                                                  "29: can't initialise variable without type or value: %",
                                                  "30: duplicate global var declaration: %",
                                                  "31: excess elements provided in array init expression: %",
                                                  "32: array initialised with 0 elements",
                                                  "33: incompatible type for array initialiser expression: %",
                                                  "34: array index expression may only be performed on array or "
                                                  "pointer types: %",
                                                  "35: type of array index must be either a signed or unsigned "
                                                  "integer: %",
                                                  "36: duplicate type declarations: %",
                                                  "37: enum declared with no fields",
                                                  "38: no such enum or import alias exists: %",
                                                  "39: no such field present on enum: %",
                                                  "40: enum declared with duplicate fields: %",
                                                  "41: unused enum: %",
                                                  "42: unknown type declared: %",
                                                  "43: statement(s) not reached",
                                                  "44: variable declared mutable but never reassigned: %",
                                                  "45: attempted reassignment of array: %",
                                                  "46: unknown array size at compile time: %",
                                                  "47: identifier declared void[]: %",
                                                  "48: function cannot return stack-allocated array: %",
                                                  "49: duplicate field declarations in class: %",
                                                  "50: class field declared void: %",
                                                  "51: class field declared void[]: %",
                                                  "52: unused class: %",
                                                  "53: unused method: %",
                                                  "54: duplicate method declaration: %",
                                                  "55: unused class constructor: %",
                                                  "56: duplicate class constructor: %",
                                                  "57: cannot return value from constructor: %",
                                                  "58: function named the same as a constructor: %",
                                                  "59: no constructor exists for provided parameters: %",
                                                  "60: may only perform field access on a class type: %",
                                                  "61: no such field exists on class type: %",
                                                  "62: field for class type must by public to access outside of class: "
                                                  "%",
                                                  "63: cannot mutate field from a const declared class identifier: %",
                                                  "64: may only perform method call on a class type: %",
                                                  "65: no method exists with that name: %",
                                                  "66: incorrect parameters for method: %",
                                                  "67: private method cannot be accessed outside of class: %",
                                                  "68: cannot access mutable method from a const declare class "
                                                  "identifier: %",
                                                  "69: cannot mutate a class field in a const declared method: %",
                                                  "70: loop lower bound must be of type i64: %",
                                                  "71: loop upper bound must be of type i64: %",
                                                  "72: 'break' must be in a loop construct",
                                                  "73: 'continue' must be in a loop construct",
                                                  "74: cannot access private function via import access: %",
                                                  "75: cannot access private class via import access: %",
                                                  "76: cannot call private constructor out of class scope",
                                                  "77: cannot access private global var via import access: %",
                                                  "78: no such global var in specified module: %",
                                                  "79: attempting to dereference a non pointer class type: %",
                                                  "80: class may only have one destructor: %",
                                                  "81: can only delete an expression of pointer or class type: %",
                                                  "82: allocation of type void or void[]",
                                                  "83: array size in allocation not of type i64: %",
                                                  "84: cannot perform a new constructor call on a non class type: %"};

    auto check_duplicate_function_declaration() -> void;
    auto check_duplicate_method_declaration(std::shared_ptr<ClassDecl>& class_decl) -> void;
    auto check_duplicate_constructor_declaration(std::shared_ptr<ClassDecl>& class_decl) -> void;
    auto check_duplicate_extern_declaration() -> void;
    auto check_duplicate_custom_type() -> void;
    auto check_duplicate_globals() -> void;
    auto check_unused_declarations() -> void;
    auto load_all_global_variables() -> void;

    auto declare_variable(std::string ident, std::shared_ptr<Decl> decl) -> void;

    auto unmurk_decl(std::shared_ptr<Decl> decl) -> void;
    auto unmurk(std::shared_ptr<Type> murky_t) -> std::shared_ptr<Type>;
    auto unmurk_direct(std::shared_ptr<MurkyType> murky_t) -> std::shared_ptr<Type>;
};

#endif // VERIFIER_HPP