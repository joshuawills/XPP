#include "verifier.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

auto SymbolTable::retrieve_one_level(std::string const& id) -> std::optional<TableEntry> {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->level != level_) {
            return std::nullopt;
        }
        if (it->id == id) {
            return *it;
        }
    }
    return std::nullopt;
}

auto Verifier::visit_para_decl(std::shared_ptr<ParaDecl> para_decl) -> void {
    declare_variable(para_decl->get_ident(), para_decl);

    if (para_decl->get_type() == handler_->VOID_TYPE) {
        handler_->report_error(current_filename_, all_errors_[4], para_decl->get_ident(), para_decl->pos());
    }
    else if (para_decl->get_type() == handler_->VARIATIC_TYPE) {
        handler_->report_error(current_filename_, all_errors_[16], para_decl->get_ident(), para_decl->pos());
    }

    return;
}

auto Verifier::visit_local_var_decl(std::shared_ptr<LocalVarDecl> local_var_decl) -> void {
    declare_variable(local_var_decl->get_ident(), local_var_decl);

    if (local_var_decl->get_type() == handler_->VOID_TYPE) {
        handler_->report_error(current_filename_, all_errors_[4], local_var_decl->get_ident(), local_var_decl->pos());
    }

    local_var_decl->get_expr()->visit(shared_from_this());
    auto const& expr_type = local_var_decl->get_expr()->get_type();
    if (local_var_decl->get_type() == handler_->UNKNOWN_TYPE) {
        local_var_decl->set_type(expr_type);
    }
    else if (local_var_decl->get_type() != expr_type) {
        auto stream = std::stringstream{};
        stream << "expected " << local_var_decl->get_type() << ", got " << expr_type;
        handler_->report_error(current_filename_, all_errors_[6], stream.str(), local_var_decl->pos());
        local_var_decl->set_type(handler_->ERROR_TYPE);
    }

    return;
}

auto Verifier::visit_extern(std::shared_ptr<Extern> extern_) -> void {
    auto i = 0u;
    auto const size = extern_->get_types().size();
    for (auto const& type : extern_->get_types()) {
        if (type == handler_->VARIATIC_TYPE) {
            extern_->set_variatic();
            if (i != size - 1) {
                handler_->report_error(current_filename_, all_errors_[17], "", extern_->pos());
                break;
            }
        }
        ++i;
    }
}

auto Verifier::visit_function(std::shared_ptr<Function> function) -> void {
    base_statement_counter = 0;
    // Verifying main function
    if (function->get_ident() == "main") {
        in_main_ = has_main_ = true;
        if (function->get_type() != handler_->VOID_TYPE) {
            auto stream = std::stringstream{};
            stream << "should return void, not " << function->get_type();
            handler_->report_error(current_filename_, all_errors_[2], stream.str(), function->pos());
        }
        else if (function->get_paras().size() != 0) {
            handler_->report_error(current_filename_, all_errors_[2], "should not have parameters", function->pos());
        }
    }

    current_function_ = function;
    symbol_table_.open_scope();
    for (auto const& para : function->get_paras()) {
        para->visit(shared_from_this());
    }
    for (auto const& stmt : function->get_stmts()) {
        stmt->visit(shared_from_this());
    }
    symbol_table_.close_scope();

    if (!has_return_ and function->get_type() != handler_->VOID_TYPE) {
        handler_->report_error(current_filename_, all_errors_[10], "in function " + function->get_ident(), function->pos());
    }

    base_statement_counter = 0;
    in_main_ = false;
}

auto Verifier::visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void {
    (void)empty_expr;
    return;
}

auto Verifier::visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void {
    auto l = assignment_expr->get_left();
    auto r = assignment_expr->get_right();
    auto res = dynamic_cast<VarExpr*>(l.get());
    if (!res) {
        handler_->report_error(current_filename_, all_errors_[7], "", assignment_expr->pos());
        assignment_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    l->visit(shared_from_this());

    if (auto ref = res->get_ref()) {
        ref->set_reassigned();
    }

    r->visit(shared_from_this());

    if (l->get_type() != handler_->ERROR_TYPE and l->get_type() != r->get_type()) {
        auto stream = std::stringstream{};
        stream << "expected " << l->get_type() << ", got " << r->get_type();
        handler_->report_error(current_filename_, all_errors_[6], stream.str(), assignment_expr->pos());
        assignment_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    assignment_expr->set_type(l->get_type());
    return;
}

auto Verifier::visit_binary_expr(std::shared_ptr<BinaryExpr> binary_expr) -> void {
    auto l = binary_expr->get_left();
    auto r = binary_expr->get_right();
    l->visit(shared_from_this());
    r->visit(shared_from_this());

    // Sub types are errors
    if (l->get_type() == handler_->ERROR_TYPE or r->get_type() == handler_->ERROR_TYPE) {
        binary_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    // "||" and "&&" operators
    if (binary_expr->get_operator() == Operator::LOGICAL_OR or binary_expr->get_operator() == Operator::LOGICAL_AND) {
        if (l->get_type() != handler_->BOOL_TYPE or r->get_type() != handler_->BOOL_TYPE) {
            auto stream = std::stringstream{};
            stream << l->get_type() << " and " << r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "==" and "!=" operators
    if (binary_expr->get_operator() == Operator::EQUAL or binary_expr->get_operator() == Operator::NOT_EQUAL) {
        auto valid_one = l->get_type() == handler_->I64_TYPE and r->get_type() == handler_->I64_TYPE;
        auto valid_two = l->get_type() == handler_->BOOL_TYPE and r->get_type() == handler_->BOOL_TYPE;
        if (!valid_one and !valid_two) {
            auto stream = std::stringstream{};
            stream << l->get_type() << " and " << r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "<", ">", "<=", ">=" operators
    if (binary_expr->get_operator() == Operator::LESS_THAN or binary_expr->get_operator() == Operator::GREATER_THAN
        or binary_expr->get_operator() == Operator::LESS_EQUAL or binary_expr->get_operator() == Operator::GREATER_EQUAL)
    {
        if (l->get_type() != handler_->I64_TYPE or r->get_type() != handler_->I64_TYPE) {
            auto stream = std::stringstream{};
            stream << l->get_type() << " and " << r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "+", "-", "*", "/" operators
    if (binary_expr->get_operator() == Operator::PLUS or binary_expr->get_operator() == Operator::MINUS
        or binary_expr->get_operator() == Operator::MULTIPLY or binary_expr->get_operator() == Operator::DIVIDE)
    {
        if (l->get_type() != handler_->I64_TYPE or r->get_type() != handler_->I64_TYPE) {
            auto stream = std::stringstream{};
            stream << l->get_type() << " and " << r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->I64_TYPE);
        }
    }
}

auto Verifier::visit_unary_expr(std::shared_ptr<UnaryExpr> unary_expr) -> void {
    auto e = unary_expr->get_expr();
    e->visit(shared_from_this());
    if (e->get_type() == handler_->ERROR_TYPE) {
        unary_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    if (unary_expr->get_operator() == Operator::NEGATE) {
        if (e->get_type() != handler_->BOOL_TYPE) {
            auto stream = std::stringstream{};
            stream << "expected a bool type, got " << e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(handler_->BOOL_TYPE);
        }
    }
    else if (unary_expr->get_operator() == Operator::PLUS or unary_expr->get_operator() == Operator::MINUS) {
        if (e->get_type() != handler_->I64_TYPE) {
            auto stream = std::stringstream{};
            stream << "expected an i64 type, got " << e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(handler_->I64_TYPE);
        }
    }

    return;
}

auto Verifier::visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void {
    (void)int_expr;
    return;
}

auto Verifier::visit_bool_expr(std::shared_ptr<BoolExpr> bool_expr) -> void {
    (void)bool_expr;
    return;
}

auto Verifier::visit_string_expr(std::shared_ptr<StringExpr> string_expr) -> void {
    (void)string_expr;
    return;
}

auto Verifier::visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void {
    auto entry = symbol_table_.retrieve_one_level(var_expr->get_name());
    if (!entry.has_value()) {
        handler_->report_error(current_filename_, all_errors_[8], var_expr->get_name(), var_expr->pos());
        var_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    var_expr->set_ref(entry->attr);
    var_expr->set_type(entry->attr->get_type());
    var_expr->get_ref()->set_used();
    return;
}

auto Verifier::visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void {
    auto const function_name = call_expr->get_name();

    if (!current_module_->function_with_name_exists(function_name)) {
        handler_->report_error(current_filename_, all_errors_[12], function_name, call_expr->pos());
        return;
    }
    if (in_main_ and function_name == "main") {
        handler_->report_error(current_filename_, all_errors_[13], "", call_expr->pos());
        return;
    }

    for (auto& arg : call_expr->get_args()) {
        arg->visit(shared_from_this());
    }

    auto equivalent_func = current_module_->get_decl(call_expr);
    if (!equivalent_func) {
        handler_->report_error(current_filename_, all_errors_[14], function_name, call_expr->pos());
        return;
    }

    (*equivalent_func)->set_used();
    call_expr->set_ref(*equivalent_func);
    call_expr->set_type((*equivalent_func)->get_type());
    return;
}

auto Verifier::visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void {
    (void)empty_stmt;
    return;
}

auto Verifier::visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void {
    local_var_stmt->get_decl()->visit(shared_from_this());
}

auto Verifier::visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void {
    has_return_ = true;
    auto expr = return_stmt->get_expr();

    expr->visit(shared_from_this());
    auto const expr_type = expr->get_type();
    if (expr_type != current_function_->get_type()) {
        auto stream = std::stringstream{};
        stream << "in function " << current_function_->get_ident() << ". expected type "
               << current_function_->get_type() << ", received " << expr_type;
        handler_->report_error(current_filename_, all_errors_[11], stream.str(), return_stmt->pos());
    }

    return;
}

auto Verifier::visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void {
    expr_stmt->get_expr()->visit(shared_from_this());
}

auto Verifier::check(std::string const& filename, bool is_main) -> void {
    current_filename_ = filename;

    // Already processed
    if (modules_->module_exists_from_filename(filename) and !is_main) {
        return;
    }

    std::shared_ptr<Module> module = nullptr;
    if (!is_main) {
        // Need to lex and parse the module first
        auto lexer = Lexer(filename, handler_);
        auto tokens = lexer.tokenize();
        auto parser = Parser(tokens, filename, handler_);
        module = parser.parse();
        modules_->add_module(module);
    }
    else {
        module = modules_->get_main_module();
    }

    current_module_ = module;

    check_duplicate_extern_declaration();
    for (auto& extern_ : current_module_->get_externs()) {
        extern_->visit(shared_from_this());
    }

    check_duplicate_function_declaration();
    for (auto& func : current_module_->get_functions()) {
        func->visit(shared_from_this());
    }

    if (is_main and !has_main_) {
        handler_->report_error(filename, all_errors_[0], "", Position{});
    }
}

auto Verifier::check_duplicate_function_declaration() -> void {
    std::vector<Function> seen_functions;
    for (const auto& func : current_module_->get_functions()) {
        auto it = std::find(seen_functions.begin(), seen_functions.end(), *func);
        if (it != seen_functions.end()) {
            handler_->report_error(current_filename_, all_errors_[1], func->get_ident(), func->pos());
        }
        else {
            seen_functions.push_back(*func);
        }
    }
}

auto Verifier::check_duplicate_extern_declaration() -> void {
    std::vector<Extern> seen_externs;
    for (const auto& extern_ : current_module_->get_externs()) {
        auto it = std::find(seen_externs.begin(), seen_externs.end(), *extern_);
        if (it != seen_externs.end()) {
            handler_->report_error(current_filename_, all_errors_[15], extern_->get_ident(), extern_->pos());
        }
        else {
            seen_externs.push_back(*extern_);
        }
    }
}

auto Verifier::declare_variable(std::string ident, std::shared_ptr<Decl> decl) -> void {
    auto entry = symbol_table_.retrieve_one_level(ident);
    if (entry.has_value()) {
        const std::string error_message = "'" + ident + "'. Previously declared at line "
                                          + std::to_string(entry->attr->pos().line_start_) + ", column "
                                          + std::to_string(entry->attr->pos().col_start_);
        if (auto derived_para_decl = std::dynamic_pointer_cast<ParaDecl>(decl)) {
            handler_->report_error(current_filename_, all_errors_[3], error_message, decl->pos());
            return;
        }
        else {
            // Assume it to be a local var for now
            handler_->report_minor_error(current_filename_, all_errors_[3], error_message, decl->pos());
            symbol_table_.remove(*entry);
        }
    }

    symbol_table_.insert(ident, decl);
}