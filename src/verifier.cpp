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

auto SymbolTable::retrieve(std::string const& id) -> std::optional<TableEntry> {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->id == id) {
            return *it;
        }
    }
    return std::nullopt;
}

auto SymbolTable::retrieve_latest_scope() -> std::vector<TableEntry> {
    auto res = std::vector<TableEntry>{};
    for (auto i = entries_.rbegin(); i != entries_.rend(); ++i) {
        if (i->level == level_) {
            res.push_back(*i);
        }
        else {
            break;
        }
    }
    return res;
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

    if (local_var_decl->get_type().is_int()) {
        current_numerical_type = local_var_decl->get_type();
    }
    local_var_decl->get_expr()->visit(shared_from_this());
    auto const& expr_type = local_var_decl->get_expr()->get_type();
    if (local_var_decl->get_type() == handler_->UNKNOWN_TYPE) {
        local_var_decl->set_type(expr_type);
    }
    else if (local_var_decl->get_type() != expr_type) {
        auto stream = std::stringstream{};
        stream << "expected " << local_var_decl->get_type() << ", got " << expr_type;
        if (local_var_decl->get_type().is_int() and expr_type.is_int()) {
            stream << ". You may require an explicit type cast";
        }
        handler_->report_error(current_filename_, all_errors_[6], stream.str(), local_var_decl->pos());
        local_var_decl->set_type(handler_->ERROR_TYPE);
    }

    current_numerical_type = std::nullopt;
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
        else if (function->get_paras().size() == 0 or function->get_paras().size() == 2) {
            auto paras = function->get_paras();
            if (paras.size() == 2) {
                if (paras[0]->get_type().get_type_spec() != TypeSpec::I32
                    or paras[1]->get_type() != handler_->CHAR_POINTER_POINTER_TYPE)
                {
                    handler_->report_error(current_filename_,
                                           all_errors_[2],
                                           "should have no parameters or an i32 and an i8**",
                                           function->pos());
                }
            }
        }
        else {
            handler_->report_error(current_filename_,
                                   all_errors_[2],
                                   "should have no parameters or an i32 and an i8**",
                                   function->pos());
        }
    }

    current_function_ = function;
    symbol_table_.open_scope();
    for (auto const& para : function->get_paras()) {
        para->visit(shared_from_this());
    }
    function->get_compound_stmt()->visit(shared_from_this());

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used()) {
                auto stream = std::stringstream{};
                stream << "local variable '" << var.attr->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], stream.str(), var.attr->pos());
            }
        }
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
    auto res = std::dynamic_pointer_cast<VarExpr>(l);
    auto deref_res = std::dynamic_pointer_cast<UnaryExpr>(l);
    if ((!res and !deref_res) or (deref_res and deref_res->get_operator() != Op::DEREF)) {
        handler_->report_error(current_filename_, all_errors_[7], "", assignment_expr->pos());
        assignment_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    l->visit(shared_from_this());

    if (res) {
        if (auto ref = res->get_ref()) {
            ref->set_reassigned();
            if (!ref->is_mut()) {
                handler_->report_error(current_filename_, all_errors_[20], res->get_name(), assignment_expr->pos());
            }
        }
    }
    else if (deref_res) {
        if (auto ref = std::dynamic_pointer_cast<VarExpr>(deref_res->get_expr())->get_ref()) {
            ref->set_reassigned();
            if (!ref->is_mut()) {
                handler_->report_error(current_filename_, all_errors_[20], ref->get_ident(), assignment_expr->pos());
            }
        }
    }

    if (l->get_type().is_int()) {
        current_numerical_type = l->get_type();
    }
    r->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    if (l->get_type() != handler_->ERROR_TYPE and r->get_type() != handler_->ERROR_TYPE
        and l->get_type() != r->get_type()) {
        auto stream = std::stringstream{};
        stream << "expected " << l->get_type() << ", got " << r->get_type();
        if (l->get_type().is_int() and r->get_type().is_int()) {
            stream << ". You may require an explicit type cast";
        }
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

    auto const l_t = l->get_type();
    auto const r_t = r->get_type();
    auto const op = binary_expr->get_operator();

    // Sub types are errors
    if (l->get_type() == handler_->ERROR_TYPE or r->get_type() == handler_->ERROR_TYPE) {
        binary_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    // "||" and "&&" operators
    if (op == Op::LOGICAL_OR or op == Op::LOGICAL_AND) {
        if (!l_t.is_bool() or !r_t.is_bool()) {
            auto stream = std::stringstream{};
            stream << l_t << " and " << r_t;
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "==" and "!=" operators
    if (op == Op::EQUAL or op == Op::NOT_EQUAL) {
        auto const valid_one = l_t.is_int() and r_t.is_int() and l_t == r_t;
        auto const valid_two = l_t.is_bool() and r_t.is_bool();
        auto const valid_three = l_t.is_pointer() and r_t.is_pointer();
        if (!valid_one and !valid_two and !valid_three) {
            auto stream = std::stringstream{};
            stream << l_t << " and " << r_t;
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "<", ">", "<=", ">=" operators
    if (op == Op::LESS_THAN or op == Op::GREATER_THAN or op == Op::LESS_EQUAL or op == Op::GREATER_EQUAL) {
        if (!l_t.is_int() or !r_t.is_int() or l_t != r_t) {
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
    if (op == Op::PLUS or op == Op::MINUS or op == Op::MULTIPLY or op == Op::DIVIDE) {
        if (l_t.is_pointer() and r_t.is_int()) {
            if (op == Op::MULTIPLY or op == Op::DIVIDE) {
                auto stream = std::stringstream{};
                stream << l->get_type() << " and " << r->get_type();
                handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            }
            binary_expr->set_pointer_arithmetic();
            binary_expr->set_type(l_t);
        }
        else if (!l_t.is_int() or !r_t.is_int() or l_t != r_t) {
            auto stream = std::stringstream{};
            stream << l->get_type() << " and " << r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(l->get_type());
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

    if (unary_expr->get_operator() == Op::NEGATE) {
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
    else if (unary_expr->get_operator() == Op::PLUS or unary_expr->get_operator() == Op::MINUS) {
        if (!e->get_type().is_int()) {
            auto stream = std::stringstream{};
            stream << "expected an i64 type, got " << e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(e->get_type());
        }
    }
    else if (unary_expr->get_operator() == Op::DEREF) {
        if (!e->get_type().is_pointer()) {
            auto stream = std::stringstream{};
            stream << "expected a pointer type, received " << e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(*e->get_type().sub_type);
        }
    }
    else if (unary_expr->get_operator() == Op::ADDRESS_OF) {
        auto const var = std::dynamic_pointer_cast<VarExpr>(e);
        if (!var) {
            handler_->report_error(current_filename_, all_errors_[25], "", unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            auto const decl = var->get_ref();
            if (!decl or !decl->is_mut()) {
                auto stream = std::stringstream{};
                stream << "variable '" << decl->get_ident() << "' defined at " << decl->pos();
                handler_->report_error(current_filename_, all_errors_[26], stream.str(), unary_expr->pos());
                unary_expr->set_type(handler_->ERROR_TYPE);
            }
            unary_expr->set_type(Type{TypeSpec::POINTER, "", std::make_shared<Type>(e->get_type())});
        }
    }

    return;
}

auto Verifier::visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void {
    if (current_numerical_type) {
        switch (current_numerical_type->get_type_spec()) {
        case I64: int_expr->set_width(64); break;
        case I32: int_expr->set_width(32); break;
        case I8: int_expr->set_width(8); break;
        default: std::cout << "UNREACHABLE Verifier::visit_int_expr";
        }
        int_expr->set_type(*current_numerical_type);
    }
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

auto Verifier::visit_char_expr(std::shared_ptr<CharExpr> char_expr) -> void {
    (void)char_expr;
    return;
}

auto Verifier::visit_var_expr(std::shared_ptr<VarExpr> var_expr) -> void {
    auto entry = symbol_table_.retrieve(var_expr->get_name());
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

    auto equivalent_func = current_module_->get_decl(call_expr);

    if (!equivalent_func) {
        handler_->report_error(current_filename_, all_errors_[14], function_name, call_expr->pos());
        return;
    }

    if (auto it = std::dynamic_pointer_cast<Function>(*equivalent_func)) {
        auto paras = it->get_paras();
        auto c = 0u;
        for (auto& arg : call_expr->get_args()) {
            if (paras[c]->get_type().is_int()) {
                current_numerical_type = paras[c]->get_type();
            }
            arg->visit(shared_from_this());
            current_numerical_type = std::nullopt;
            ++c;
        }
    }
    else if (auto it = std::dynamic_pointer_cast<Extern>(*equivalent_func)) {
        auto types = it->get_types();
        auto c = 0u;
        for (auto& arg : call_expr->get_args()) {
            if (types[c].is_int()) {
                current_numerical_type = types[c];
            }
            arg->visit(shared_from_this());
            current_numerical_type = std::nullopt;
            ++c;
        }
    }

    (*equivalent_func)->set_used();
    call_expr->set_ref(*equivalent_func);
    call_expr->set_type((*equivalent_func)->get_type());
    return;
}

auto Verifier::visit_cast_expr(std::shared_ptr<CastExpr> cast_expr) -> void {
    cast_expr->get_expr()->visit(shared_from_this());
    if (!cast_expr->get_expr()->get_type().equal_soft(cast_expr->get_to_type())) {
        auto stream = std::stringstream{};
        stream << "expected " << cast_expr->get_to_type().t << ", received " << cast_expr->get_expr()->get_type();
        handler_->report_error(current_filename_, all_errors_[27], stream.str(), cast_expr->pos());
        cast_expr->set_type(handler_->ERROR_TYPE);
    }
    return;
}

auto Verifier::visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void {
    (void)empty_stmt;
    return;
}

auto Verifier::visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void {
    symbol_table_.open_scope();
    for (auto& stmt : compound_stmt->get_stmts()) {
        stmt->visit(shared_from_this());
    }

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used()) {
                auto stream = std::stringstream{};
                stream << "local variable '" << var.attr->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], stream.str(), var.attr->pos());
            }
        }
    }

    symbol_table_.close_scope();
    return;
}

auto Verifier::visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void {
    local_var_stmt->get_decl()->visit(shared_from_this());
}

auto Verifier::visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void {
    has_return_ = true;
    auto expr = return_stmt->get_expr();

    if (current_function_->get_type().is_int()) {
        current_numerical_type = current_function_->get_type();
    }
    expr->visit(shared_from_this());
    current_numerical_type = std::nullopt;

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

auto Verifier::visit_while_stmt(std::shared_ptr<WhileStmt> while_stmt) -> void {
    auto cond = while_stmt->get_cond();
    cond->visit(shared_from_this());

    if (cond->get_type() != handler_->BOOL_TYPE) {
        auto stream = std::stringstream{};
        stream << "received ";
        stream << cond->get_type().get_type_spec();
        handler_->report_error(current_filename_, all_errors_[19], stream.str(), cond->pos());
    }

    while_stmt->get_stmts()->visit(shared_from_this());
    return;
}

auto Verifier::visit_if_stmt(std::shared_ptr<IfStmt> if_stmt) -> void {
    auto cond = if_stmt->get_cond();
    cond->visit(shared_from_this());
    if (cond->get_type() != handler_->BOOL_TYPE) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type().get_type_spec();
        handler_->report_error(current_filename_, all_errors_[24], stream.str(), cond->pos());
    }

    if_stmt->get_body_stmt()->visit(shared_from_this());

    if_stmt->get_else_if_stmt()->visit(shared_from_this());

    if_stmt->get_else_stmt()->visit(shared_from_this());

    return;
}

auto Verifier::visit_else_if_stmt(std::shared_ptr<ElseIfStmt> else_if_stmt) -> void {
    auto cond = else_if_stmt->get_cond();
    cond->visit(shared_from_this());
    if (cond->get_type() != handler_->BOOL_TYPE) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type().get_type_spec();
        handler_->report_error(current_filename_, all_errors_[24], stream.str(), cond->pos());
    }

    else_if_stmt->get_body_stmt()->visit(shared_from_this());

    else_if_stmt->get_nested_else_if_stmt()->visit(shared_from_this());

    return;
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

    if (!handler_->quiet_mode()) {
        check_unused_declarations();
    }

    if (is_main and !has_main_) {
        handler_->report_error(filename, all_errors_[0], "", Position{});
    }
}

auto Verifier::check_unused_declarations() -> void {
    for (auto const& module : modules_->get_modules()) {
        for (auto const& func : module->get_functions()) {
            if (func->get_ident() != "main" and !func->is_used()) {
                auto stream = std::stringstream{};
                stream << "'" << func->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[22], stream.str(), func->pos());
            }
        }
        for (auto const& extern_ : module->get_externs()) {
            if (!extern_->is_used()) {
                auto stream = std::stringstream{};
                stream << "'" << extern_->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[23], stream.str(), extern_->pos());
            }
        }
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
            handler_->report_error(current_filename_, all_errors_[3], error_message, decl->pos());
            symbol_table_.remove(*entry);
        }
    }

    symbol_table_.insert(ident, decl);
}