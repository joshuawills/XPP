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
    auto visit_function(std::shared_ptr<Function> function) -> void override;
    auto visit_extern(std::shared_ptr<Extern> extern_) -> void override;
    auto visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void override;
    auto visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void override;
    auto visit_binary_expr(std::shared_ptr<BinaryExpr> binary_expr) -> void override;
    auto visit_unary_expr(std::shared_ptr<UnaryExpr> unary_expr) -> void override;
    auto visit_decimal_expr(std::shared_ptr<DecimalExpr> decimal_expr) -> void override;
    auto visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void override;
    auto visit_uint_expr(std::shared_ptr<UIntExpr> uint_expr) -> void override;
    auto visit_bool_expr(std::shared_ptr<BoolExpr> bool_expr) -> void override;
    auto visit_string_expr(std::shared_ptr<StringExpr> string_expr) -> void override;
    auto visit_char_expr(std::shared_ptr<CharExpr> char_expr) -> void override;
    auto visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void override;
    auto visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void override;
    auto visit_cast_expr(std::shared_ptr<CastExpr> cast_expr) -> void override;
    auto visit_array_init_expr(std::shared_ptr<ArrayInitExpr> array_init_expr) -> void override;
    auto visit_array_index_expr(std::shared_ptr<ArrayIndexExpr> array_index_expr) -> void override;
    auto visit_enum_access_expr(std::shared_ptr<EnumAccessExpr> enum_access_expr) -> void override;

    auto visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void override;
    auto visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void override;
    auto visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void override;
    auto visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void override;
    auto visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void override;
    auto visit_while_stmt(std::shared_ptr<WhileStmt> while_stmt) -> void override;
    auto visit_if_stmt(std::shared_ptr<IfStmt> if_stmt) -> void override;
    auto visit_else_if_stmt(std::shared_ptr<ElseIfStmt> else_if_stmt) -> void override;

    auto check(std::string const& filename, bool is_main) -> void;

    std::optional<std::shared_ptr<Type>> current_numerical_type = std::nullopt;
    Position unmurk_pos;

 private:
    std::shared_ptr<Handler> handler_;
    std::shared_ptr<AllModules> modules_;

    SymbolTable symbol_table_ = {};

    bool has_main_ = false, has_return_ = false, in_main_ = false;

    size_t global_statement_counter_ = 0, loop_depth_ = 0;

    std::string current_filename_;
    std::shared_ptr<Module> current_module_ = nullptr;
    std::shared_ptr<Function> current_function_ = nullptr;

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
                                                  "36: duplicate enum declarations: %",
                                                  "37: enum declared with no fields",
                                                  "38: no such enum exists: %",
                                                  "39: no such field present on enum: %",
                                                  "40: enum declared with duplicate fields: %",
                                                  "41: unused enum: %",
                                                  "42: unknown type declared: %"};

    auto check_duplicate_function_declaration() -> void;
    auto check_duplicate_extern_declaration() -> void;
    auto check_duplicate_enums() -> void;
    auto check_duplicate_globals() -> void;
    auto check_unused_declarations() -> void;
    auto load_all_global_variables() -> void;

    auto declare_variable(std::string ident, std::shared_ptr<Decl> decl) -> void;

    auto unmurk_decl(std::shared_ptr<Decl> decl) -> void;
    auto unmurk(std::shared_ptr<Type> murky_t) -> std::shared_ptr<Type>;
    auto unmurk_direct(std::shared_ptr<MurkyType> murky_t) -> std::shared_ptr<Type>;
};

#endif // VERIFIER_HPP