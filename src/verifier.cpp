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
        auto dot_pos = it->id.find('.');
        auto prefix = it->id.substr(0, dot_pos);
        if (prefix == id) {
            return *it;
        }
    }
    return std::nullopt;
}

auto SymbolTable::retrieve(std::string const& id) -> std::optional<TableEntry> {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        auto dot_pos = it->id.find('.');
        auto prefix = it->id.substr(0, dot_pos);
        if (prefix == id) {
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
    para_decl->set_statement_num(global_statement_counter_);
    para_decl->set_depth_num(loop_depth_);
    declare_variable(para_decl->get_ident() + para_decl->get_append(), para_decl);

    if (para_decl->get_type()->is_void()) {
        handler_->report_error(current_filename_, all_errors_[4], para_decl->get_ident(), para_decl->pos());
    }
    else if (para_decl->get_type()->is_variatic()) {
        handler_->report_error(current_filename_, all_errors_[16], para_decl->get_ident(), para_decl->pos());
    }

    return;
}

auto Verifier::visit_local_var_decl(std::shared_ptr<LocalVarDecl> local_var_decl) -> void {
    unmurk_decl(local_var_decl);

    local_var_decl->set_statement_num(global_statement_counter_);
    local_var_decl->set_depth_num(loop_depth_);
    declare_variable(local_var_decl->get_ident() + local_var_decl->get_append(), local_var_decl);

    if (local_var_decl->get_type()->is_void()) {
        handler_->report_error(current_filename_, all_errors_[4], local_var_decl->get_ident(), local_var_decl->pos());
    }

    if (local_var_decl->get_type()->is_numeric()) {
        current_numerical_type = local_var_decl->get_type();
    }
    local_var_decl->get_expr()->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    auto const& expr_type = local_var_decl->get_expr()->get_type();
    auto const has_expr = !(std::dynamic_pointer_cast<EmptyExpr>(local_var_decl->get_expr()));

    if (local_var_decl->get_type()->is_unknown()) {
        if (expr_type->is_void()) {
            handler_->report_error(current_filename_, all_errors_[29], local_var_decl->get_ident(), local_var_decl->pos());
            local_var_decl->set_type(handler_->ERROR_TYPE);
        }
        else {
            local_var_decl->set_type(expr_type);
        }
    }
    else if (has_expr and !expr_type->is_error() and *local_var_decl->get_type() != *expr_type) {
        // Implicit casting from array to pointer in assignment expressions
        auto r = local_var_decl->get_expr();
        if (local_var_decl->get_type()->is_pointer() and r->get_type()->is_array()) {
            auto p_l = std::dynamic_pointer_cast<PointerType>(local_var_decl->get_type());
            auto a_r = std::dynamic_pointer_cast<ArrayType>(r->get_type());
            if (*p_l->get_sub_type() != *a_r->get_sub_type()) {
                auto stream = std::stringstream{};
                stream << "expected " << *p_l->get_sub_type() << " as an inner type, got " << *a_r->get_sub_type();
                stream << ". You can cast from array to pointer, but the inner types must remain the same";
                handler_->report_error(current_filename_, all_errors_[6], stream.str(), local_var_decl->pos());
                local_var_decl->set_type(handler_->ERROR_TYPE);
            }
            else {
                local_var_decl->set_type(local_var_decl->get_type());
            }
            return;
        }

        auto stream = std::stringstream{};
        stream << "expected " << *local_var_decl->get_type() << ", got " << *expr_type;
        if (local_var_decl->get_type()->is_numeric() and expr_type->is_numeric()) {
            stream << ". You may require an explicit type cast";
        }
        if (local_var_decl->get_type()->is_unsigned_int()) {
            stream << ". Note that unsigned integer literals should end with a 'u'.";
        }
        handler_->report_error(current_filename_, all_errors_[6], stream.str(), local_var_decl->pos());
        local_var_decl->set_type(handler_->ERROR_TYPE);
    }

    std::shared_ptr<ArrayType> l;
    if (has_expr and (l = std::dynamic_pointer_cast<ArrayType>(expr_type))) {
        local_var_decl->set_type(l);
    }
    if (local_var_decl->get_type()->is_array() and !expr_type->is_error()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(local_var_decl->get_type());
        if (!a_t->get_length().has_value()) {
            handler_->report_error(current_filename_, all_errors_[46], local_var_decl->get_ident(), local_var_decl->pos());
            local_var_decl->set_type(handler_->ERROR_TYPE);
        }
    }

    return;
}

auto Verifier::visit_global_var_decl(std::shared_ptr<GlobalVarDecl> global_var_decl) -> void {
    auto const name = global_var_decl->get_ident();

    if (global_var_decl->get_type()->is_void()) {
        handler_->report_error(current_filename_, all_errors_[4], name, global_var_decl->pos());
    }
    if (global_var_decl->get_type()->is_numeric()) {
        current_numerical_type = global_var_decl->get_type();
    }
    global_var_decl->get_expr()->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    auto const& expr_type = global_var_decl->get_expr()->get_type();
    auto const has_expr = !(std::dynamic_pointer_cast<EmptyExpr>(global_var_decl->get_expr()));
    if (global_var_decl->get_type()->is_unknown()) {
        if (expr_type->is_void()) {
            handler_->report_error(current_filename_, all_errors_[29], name, global_var_decl->pos());
            global_var_decl->set_type(handler_->ERROR_TYPE);
        }
        else {
            global_var_decl->set_type(expr_type);
        }
    }
    else if (has_expr and *global_var_decl->get_type() != *expr_type) {
        auto stream = std::stringstream{};
        stream << "expected " << global_var_decl->get_type() << ", got " << expr_type;
        if (global_var_decl->get_type()->is_numeric() and expr_type->is_numeric()) {
            stream << ". You may require an explicit type cast";
        }
        if (global_var_decl->get_type()->is_unsigned_int()) {
            stream << ". Note that unsigned integer literals should end with a 'u'.";
        }
        handler_->report_error(current_filename_, all_errors_[6], stream.str(), global_var_decl->pos());
        global_var_decl->set_type(handler_->ERROR_TYPE);
    }

    std::shared_ptr<ArrayType> l;
    if (has_expr and (l = std::dynamic_pointer_cast<ArrayType>(expr_type))) {
        global_var_decl->set_type(l);
    }
    if (global_var_decl->get_type()->is_array() and !expr_type->is_error()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(global_var_decl->get_type());
        if (!a_t->get_length().has_value()) {
            handler_->report_error(current_filename_,
                                   all_errors_[46],
                                   global_var_decl->get_ident(),
                                   global_var_decl->pos());
            global_var_decl->set_type(handler_->ERROR_TYPE);
        }
    }

    return;
}

auto Verifier::visit_enum_decl(std::shared_ptr<EnumDecl> enum_decl) -> void {
    if (enum_decl->get_fields().size() == 0) {
        handler_->report_error(current_filename_, all_errors_[37], enum_decl->get_ident(), enum_decl->pos());
    }
    auto duplicates = enum_decl->find_duplicates();
    if (duplicates.size() > 0) {
        auto err = "fields on enum '" + enum_decl->get_ident() + "' are duplicated: ";
        for (auto i = 0u; i < duplicates.size(); ++i) {
            err += duplicates[i];
            if (duplicates.size() > 1) {
                if (i == duplicates.size() - 2) {
                    err += " and ";
                }
                else if (i < duplicates.size() - 2) {
                    err += ", ";
                }
            }
        }
        handler_->report_error(current_filename_, all_errors_[40], err, enum_decl->pos());
    }
}

auto Verifier::visit_extern(std::shared_ptr<Extern> extern_) -> void {
    auto i = 0u;
    auto const size = extern_->get_types().size();
    for (auto const& type : extern_->get_types()) {
        if (type->is_variatic()) {
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
    global_statement_counter_ = 0;
    // Verifying main function
    if (function->get_ident() == "main") {
        in_main_ = has_main_ = true;
        if (!function->get_type()->is_void()) {
            auto stream = std::stringstream{};
            stream << "should return void, not " << function->get_type();
            handler_->report_error(current_filename_, all_errors_[2], stream.str(), function->pos());
        }
        else if (function->get_paras().size() == 0 or function->get_paras().size() == 2) {
            auto paras = function->get_paras();
            auto const char_p_t = std::make_shared<PointerType>(std::make_shared<Type>(TypeSpec::I8));
            if (paras.size() == 2) {
                if (paras[0]->get_type()->get_type_spec() != TypeSpec::I32 or *paras[1]->get_type() != *char_p_t) {
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
        // or if they were declared mutable but never reassigned
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used()) {
                auto err = "local variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], err, var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned()) {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    symbol_table_.close_scope();

    if (!has_return_ and !function->get_type()->is_void()) {
        if (!function->get_type()->is_error()) {
            handler_->report_error(current_filename_,
                                   all_errors_[10],
                                   "in function " + function->get_ident(),
                                   function->pos());
        }
    }

    global_statement_counter_ = 0;
    in_main_ = false;
}

auto Verifier::visit_empty_expr(std::shared_ptr<EmptyExpr> empty_expr) -> void {
    (void)empty_expr;
    return;
}

auto Verifier::visit_assignment_expr(std::shared_ptr<AssignmentExpr> assignment_expr) -> void {
    auto const op = assignment_expr->get_operator();
    auto l = assignment_expr->get_left();
    auto r = assignment_expr->get_right();
    auto res = std::dynamic_pointer_cast<VarExpr>(l);
    auto deref_res = std::dynamic_pointer_cast<UnaryExpr>(l);
    auto array_index_res = std::dynamic_pointer_cast<ArrayIndexExpr>(l);
    if ((!res and !deref_res and !array_index_res) or (deref_res and deref_res->get_operator() != Op::DEREF)) {
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
            if (l->get_type()->is_array()) {
                handler_->report_error(current_filename_, all_errors_[45], res->get_name(), assignment_expr->pos());
                assignment_expr->set_type(handler_->ERROR_TYPE);
                return;
            }
        }
    }
    else if (deref_res) {
        if (auto ref = std::dynamic_pointer_cast<VarExpr>(deref_res->get_expr())->get_ref()) {
            ref->set_reassigned();
            if (!ref->is_mut()) {
                handler_->report_error(current_filename_, all_errors_[20], ref->get_ident(), assignment_expr->pos());
            }
            if (l->get_type()->is_array()) {
                handler_->report_error(current_filename_, all_errors_[45], ref->get_ident(), assignment_expr->pos());
                assignment_expr->set_type(handler_->ERROR_TYPE);
                return;
            }
        }
    }
    else if (array_index_res) {
        if (auto ref = std::dynamic_pointer_cast<VarExpr>(array_index_res->get_array_expr())->get_ref()) {
            ref->set_reassigned();
            if (!ref->is_mut()) {
                handler_->report_error(current_filename_, all_errors_[20], ref->get_ident(), assignment_expr->pos());
            }
            if (l->get_type()->is_array()) {
                handler_->report_error(current_filename_, all_errors_[45], ref->get_ident(), assignment_expr->pos());
                assignment_expr->set_type(handler_->ERROR_TYPE);
                return;
            }
        }
    }

    if (l->get_type()->is_numeric()) {
        current_numerical_type = l->get_type();
    }
    r->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    // Implicit casting from array to pointer in assignment expressions
    if (l->get_type()->is_pointer() and r->get_type()->is_array()) {
        auto p_l = std::dynamic_pointer_cast<PointerType>(l->get_type());
        auto a_r = std::dynamic_pointer_cast<ArrayType>(r->get_type());
        if (*p_l->get_sub_type() != *a_r->get_sub_type()) {
            auto stream = std::stringstream{};
            stream << "expected " << *p_l->get_sub_type() << " as an inner type, got " << *a_r->get_sub_type();
            stream << ". You can cast from array to pointer, but the inner types must remain the same";
            handler_->report_error(current_filename_, all_errors_[6], stream.str(), assignment_expr->pos());
            assignment_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            assignment_expr->set_type(l->get_type());
        }
        return;
    }

    if (!l->get_type()->is_error() and !r->get_type()->is_error() and *l->get_type() != *r->get_type()) {
        auto const special_op = op == Op::PLUS_ASSIGN or op == Op::MINUS_ASSIGN;
        auto const non_special_op = op == Op::MULTIPLY_ASSIGN or op == Op::DIVIDE_ASSIGN;
        if (!(special_op and l->get_type()->is_pointer() and r->get_type()->is_int())) {
            auto stream = std::stringstream{};
            if (non_special_op) {
                stream << "*= and /= can't be applied to pointer types";
            }
            else {
                stream << "expected " << *l->get_type() << ", got " << *r->get_type();
                if (l->get_type()->is_numeric() and r->get_type()->is_numeric()) {
                    stream << ". You may require an explicit type cast";
                }
                if (l->get_type()->is_unsigned_int()) {
                    stream << ". Note that unsigned integer literals should end with a 'u'.";
                }
            }
            handler_->report_error(current_filename_, all_errors_[6], stream.str(), assignment_expr->pos());
            assignment_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
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
    if (l->get_type()->is_error() or r->get_type()->is_error()) {
        binary_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    // "||" and "&&" operators
    if (op == Op::LOGICAL_OR or op == Op::LOGICAL_AND) {
        if (!l_t->is_bool() or !r_t->is_bool()) {
            auto stream = std::stringstream{};
            stream << *l_t << " and " << *r_t;
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "==" and "!=" operators
    if (op == Op::EQUAL or op == Op::NOT_EQUAL) {
        auto const valid_one = l_t->is_numeric() and r_t->is_numeric() and *l_t == *r_t;
        auto const valid_two = l_t->is_bool() and r_t->is_bool();
        auto const valid_three = l_t->is_pointer() and r_t->is_pointer();
        auto const valid_four = l_t->is_enum() and r_t->is_enum();
        if (!valid_one and !valid_two and !valid_three and !valid_four) {
            auto stream = std::stringstream{};
            stream << *l_t << " and " << *r_t;
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "<", ">", "<=", ">=" operators
    if (op == Op::LESS_THAN or op == Op::GREATER_THAN or op == Op::LESS_EQUAL or op == Op::GREATER_EQUAL) {
        if (!(l_t->is_numeric() and r_t->is_numeric() and *l_t == *r_t)) {
            auto stream = std::stringstream{};
            stream << *l->get_type() << " and " << *r->get_type();
            handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            binary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            binary_expr->set_type(handler_->BOOL_TYPE);
        }
    }

    // "+", "-", "*", "/" operators
    if (op == Op::PLUS or op == Op::MINUS or op == Op::MULTIPLY or op == Op::DIVIDE or op == Op::MODULO) {
        if (l_t->is_pointer() and r_t->is_int()) {
            if (op == Op::MULTIPLY or op == Op::DIVIDE) {
                auto stream = std::stringstream{};
                stream << *l->get_type() << " and " << *r->get_type();
                handler_->report_error(current_filename_, all_errors_[5], stream.str(), binary_expr->pos());
            }
            binary_expr->set_pointer_arithmetic();
            binary_expr->set_type(l_t);
        }
        else if (!(l_t->is_numeric() and r_t->is_numeric() and *l_t == *r_t)) {
            auto stream = std::stringstream{};
            stream << *l->get_type() << " and " << *r->get_type();
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
    auto op = unary_expr->get_operator();
    e->visit(shared_from_this());
    if (e->get_type()->is_error()) {
        unary_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    if (op == Op::NEGATE) {
        if (!e->get_type()->is_bool()) {
            auto stream = std::stringstream{};
            stream << "expected a bool type, got " << *e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(handler_->BOOL_TYPE);
        }
    }
    else if (op == Op::PREFIX_ADD or op == Op::PREFIX_MINUS or op == Op::POSTFIX_ADD or op == Op::POSTFIX_MINUS) {
        if (!e->get_type()->is_numeric() and !e->get_type()->is_pointer()) {
            auto stream = std::stringstream{};
            stream << "expected a numeric type, got " << *e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
        auto is_mut = false;
        auto is_lvalue = false;
        auto var_name = std::string{};
        if (auto l = std::dynamic_pointer_cast<VarExpr>(e)) {
            is_mut |= l->get_ref()->is_mut();
            is_lvalue = true;
            var_name = l->get_name();
        }
        if (auto l = std::dynamic_pointer_cast<UnaryExpr>(e)) {
            if (l->get_operator() == Op::DEREF) {
                auto v = std::dynamic_pointer_cast<VarExpr>(l->get_expr());
                is_mut |= v->get_ref()->is_mut();
                is_lvalue = true;
                var_name = v->get_name();
            }
        }
        if (!is_lvalue) {
            handler_->report_error(current_filename_, all_errors_[28], "", unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else if (!is_mut) {
            handler_->report_error(current_filename_, all_errors_[20], var_name, unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(e->get_type());
        }
    }
    else if (op == Op::PLUS or op == Op::MINUS) {
        if (!e->get_type()->is_numeric()) {
            auto stream = std::stringstream{};
            stream << "expected a numeric type, got " << *e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            unary_expr->set_type(e->get_type());
        }
    }
    else if (op == Op::DEREF) {
        if (!e->get_type()->is_pointer()) {
            auto stream = std::stringstream{};
            stream << "expected a pointer type, received " << *e->get_type();
            handler_->report_error(current_filename_, all_errors_[9], stream.str(), unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else {
            auto const p_t = std::dynamic_pointer_cast<PointerType>(e->get_type());
            unary_expr->set_type(p_t->get_sub_type());
        }
    }
    else if (op == Op::ADDRESS_OF) {
        auto const var = std::dynamic_pointer_cast<VarExpr>(e);
        auto const index = std::dynamic_pointer_cast<ArrayIndexExpr>(e);
        if (!var and !index) {
            handler_->report_error(current_filename_, all_errors_[25], "", unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else if (var) {
            auto const decl = var->get_ref();
            if (!decl or !decl->is_mut()) {
                auto stream = std::stringstream{};
                stream << "variable '" << decl->get_ident() << "' defined at " << decl->pos();
                handler_->report_error(current_filename_, all_errors_[26], stream.str(), unary_expr->pos());
                unary_expr->set_type(handler_->ERROR_TYPE);
            }
            unary_expr->set_type(std::make_shared<PointerType>(e->get_type()));
        }
        else if (index) {
            auto const var_expr = std::dynamic_pointer_cast<VarExpr>(index->get_array_expr());
            if (var_expr and !var_expr->get_ref()->is_mut()) {
                auto stream = std::stringstream{};
                stream << "array '" << var_expr->get_name() << "' defined at " << var_expr->get_ref()->pos();
                handler_->report_error(current_filename_, all_errors_[26], stream.str(), unary_expr->pos());
                unary_expr->set_type(handler_->ERROR_TYPE);
            }
            unary_expr->get_expr()->set_parent(unary_expr);
            unary_expr->set_type(std::make_shared<PointerType>(e->get_type()));
        }
        else {
            std::cout << "UNREACHABLE Verifier::visit_unary_expr\n";
        }
    }

    return;
}

auto Verifier::visit_int_expr(std::shared_ptr<IntExpr> int_expr) -> void {
    if (current_numerical_type.has_value() and !(*current_numerical_type)->is_signed_int()) {
        int_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    if (current_numerical_type) {
        switch ((*current_numerical_type)->get_type_spec()) {
        case I64: int_expr->set_width(64); break;
        case I32: int_expr->set_width(32); break;
        case I8: int_expr->set_width(8); break;
        default: std::cout << "UNREACHABLE Verifier::visit_int_expr";
        }
        int_expr->set_type(*current_numerical_type);
    }
    return;
}

auto Verifier::visit_decimal_expr(std::shared_ptr<DecimalExpr> decimal_expr) -> void {
    if (current_numerical_type.has_value() and !(*current_numerical_type)->is_decimal()) {
        decimal_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    if (current_numerical_type) {
        switch ((*current_numerical_type)->get_type_spec()) {
        case F64: decimal_expr->set_width(64); break;
        case F32: decimal_expr->set_width(32); break;
        default: std::cout << "UNREACHABLE Verifier::visit_decimal_expr";
        }
        decimal_expr->set_type(*current_numerical_type);
    }
    return;
}

auto Verifier::visit_uint_expr(std::shared_ptr<UIntExpr> uint_expr) -> void {
    if (current_numerical_type.has_value() and !(*current_numerical_type)->is_unsigned_int()) {
        return;
    }

    if (current_numerical_type) {
        switch ((*current_numerical_type)->get_type_spec()) {
        case U64: uint_expr->set_width(64); break;
        case U32: uint_expr->set_width(32); break;
        case U8: uint_expr->set_width(8); break;
        default: std::cout << "UNREACHABLE Verifier::visit_uint_expr";
        }
        uint_expr->set_type(*current_numerical_type);
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

    for (auto& arg : call_expr->get_args()) {
        arg->visit(shared_from_this());
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
            if (paras[c]->get_type()->is_numeric()) {
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
            if (c < types.size() and types[c]->is_numeric()) {
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
    auto expr = cast_expr->get_expr();
    auto to_type = cast_expr->get_to_type();
    expr->visit(shared_from_this());
    auto const valid_one = expr->get_type()->is_numeric() and to_type->is_numeric();
    if (!valid_one) {
        auto stream = std::stringstream{};
        stream << "expected " << cast_expr->get_to_type()->get_type_spec() << ", received "
               << *cast_expr->get_expr()->get_type();
        handler_->report_error(current_filename_, all_errors_[27], stream.str(), cast_expr->pos());
        cast_expr->set_type(handler_->ERROR_TYPE);
    }
    return;
}

auto Verifier::visit_array_init_expr(std::shared_ptr<ArrayInitExpr> array_init_expr) -> void {
    auto const arg_count = array_init_expr->get_exprs().size();
    auto p = array_init_expr->get_parent();
    auto has_size_specified = false;
    auto size_specified = size_t{0};
    auto has_sub_type_specified = false;
    std::shared_ptr<Type> parent_t;
    std::shared_ptr<Type> individual_type;
    auto error_occured = false;

    if (auto l = std::dynamic_pointer_cast<LocalVarDecl>(p)) {
        parent_t = l->get_type();
    }
    else if (auto g = std::dynamic_pointer_cast<GlobalVarDecl>(p)) {
        parent_t = g->get_type();
    }

    if (parent_t and parent_t->is_array()) {
        if (auto l = std::dynamic_pointer_cast<ArrayType>(parent_t)) {
            has_sub_type_specified = true;
            individual_type = l->get_sub_type();
            if (individual_type->is_numeric()) {
                current_numerical_type = parent_t;
            }
            auto len = l->get_length();
            if (len.has_value()) {
                has_size_specified = true;
                size_specified = *len;
            }
        }
    }

    auto element_types = individual_type;
    auto elem_num = 0u;
    for (auto& expr : array_init_expr->get_exprs()) {
        expr->visit(shared_from_this());
        if (elem_num == 0 and !has_sub_type_specified) {
            element_types = expr->get_type();
        }

        if (!expr->get_type()->is_error() and *expr->get_type() != *element_types) {
            auto stream = std::stringstream{};
            stream << "position " << elem_num << ". Expected " << *element_types << ", got " << *expr->get_type();
            if (element_types->is_numeric() and expr->get_type()->is_numeric()) {
                stream << ". You may require an explicit type cast";
            }
            if (element_types->is_numeric()) {
                stream << ". Note that unsigned integer literals should end with a 'u'.";
            }
            handler_->report_error(current_filename_, all_errors_[33], stream.str(), expr->pos());
            error_occured = true;
        }
        ++elem_num;
    }

    current_numerical_type = std::nullopt;

    if (has_size_specified and arg_count > size_specified) {
        handler_->report_error(current_filename_,
                               all_errors_[31],
                               "expected " + std::to_string(size_specified) + ", received " + std::to_string(arg_count),
                               array_init_expr->pos());
        error_occured = true;
    }
    else if ((has_size_specified and size_specified == 0) or (!has_size_specified and arg_count == 0)) {
        handler_->report_error(current_filename_, all_errors_[32], "", array_init_expr->pos());
        error_occured = true;
    }

    if (error_occured) {
        array_init_expr->set_type(handler_->ERROR_TYPE);
    }
    else {
        array_init_expr->set_type(
            std::make_shared<ArrayType>(element_types, (has_size_specified) ? size_specified : arg_count));
    }

    return;
}

auto Verifier::visit_array_index_expr(std::shared_ptr<ArrayIndexExpr> array_index_expr) -> void {
    auto has_error = false;
    array_index_expr->get_array_expr()->visit(shared_from_this());
    array_index_expr->get_array_expr()->set_parent(array_index_expr);
    auto const array_expr_t = array_index_expr->get_array_expr()->get_type();

    if (!array_expr_t->is_array() and !array_expr_t->is_pointer()) {
        auto stream = std::stringstream{};
        stream << "received type " << array_expr_t;
        handler_->report_error(current_filename_, all_errors_[34], stream.str(), array_index_expr->pos());
        has_error = true;
    }

    array_index_expr->get_index_expr()->visit(shared_from_this());
    auto const array_index_t = array_index_expr->get_index_expr()->get_type();
    if (!array_index_t->is_int()) {
        auto stream = std::stringstream{};
        stream << "received type " << *array_index_t;
        handler_->report_error(current_filename_, all_errors_[35], stream.str(), array_index_expr->pos());
        has_error = true;
    }
    else if (!array_index_t->is_i64()) {
        auto const old_expr = array_index_expr->get_index_expr();
        auto new_expr = std::make_shared<CastExpr>(old_expr->pos(), old_expr, std::make_shared<Type>(TypeSpec::I64));
        array_index_expr->set_index_expr(new_expr);
    }

    if (!has_error) {
        if (auto l = std::dynamic_pointer_cast<ArrayType>(array_expr_t)) {
            array_index_expr->set_type(l->get_sub_type());
        }
        else if (auto l2 = std::dynamic_pointer_cast<PointerType>(array_expr_t)) {
            array_index_expr->set_type(l2->get_sub_type());
        }
    }
    else {
        array_index_expr->set_type(handler_->ERROR_TYPE);
    }

    return;
}

auto Verifier::visit_enum_access_expr(std::shared_ptr<EnumAccessExpr> enum_access_expr) -> void {
    auto enum_name = enum_access_expr->get_enum_name();
    auto field_name = enum_access_expr->get_field();
    auto ref = current_module_->get_enum(enum_name);
    if (!ref) {
        handler_->report_error(current_filename_, all_errors_[38], enum_name, enum_access_expr->pos());
        enum_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    auto num = (*ref)->get_num(field_name);
    if (!num) {
        handler_->report_error(current_filename_,
                               all_errors_[39],
                               "field '" + field_name + "' on enum " + enum_name,
                               enum_access_expr->pos());
        enum_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    (*ref)->set_used();
    enum_access_expr->set_type(std::make_shared<EnumType>(*ref));
    enum_access_expr->set_field_num(*num);
}

auto Verifier::visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void {
    (void)empty_stmt;
    return;
}

auto Verifier::visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void {
    auto p = compound_stmt->get_parent();
    auto parent_is_function = p != nullptr and std::dynamic_pointer_cast<Function>(p);

    if (!parent_is_function) {
        symbol_table_.open_scope();
    }
    auto i = 0u;
    auto const s = compound_stmt->get_stmts().size();
    for (auto& stmt : compound_stmt->get_stmts()) {
        ++global_statement_counter_;
        stmt->visit(shared_from_this());
        if (i != s - 1 and !handler_->quiet_mode()) {
            auto const is_return_stmt = std::dynamic_pointer_cast<ReturnStmt>(stmt);
            if (is_return_stmt) {
                handler_->report_error(current_filename_, all_errors_[43], "", stmt->pos());
            }
        }
        ++i;
    }
    if (!parent_is_function and !handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used()) {
                auto stream = std::stringstream{};
                stream << "local variable '" << var.attr->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], stream.str(), var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned()) {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    if (!parent_is_function) {
        symbol_table_.close_scope();
    }
    return;
}

auto Verifier::visit_local_var_stmt(std::shared_ptr<LocalVarStmt> local_var_stmt) -> void {
    local_var_stmt->get_decl()->visit(shared_from_this());
}

auto Verifier::visit_return_stmt(std::shared_ptr<ReturnStmt> return_stmt) -> void {
    has_return_ = true;
    auto expr = return_stmt->get_expr();

    if (current_function_->get_type()->is_numeric()) {
        current_numerical_type = current_function_->get_type();
    }
    expr->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    auto const expr_type = expr->get_type();
    if (*expr_type != *current_function_->get_type()) {
        auto stream = std::stringstream{};
        stream << "in function " << current_function_->get_ident() << ". expected type "
               << *current_function_->get_type() << ", received " << *expr_type;
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

    if (!cond->get_type()->is_bool()) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type()->get_type_spec();
        handler_->report_error(current_filename_, all_errors_[19], stream.str(), cond->pos());
    }

    while_stmt->get_stmts()->visit(shared_from_this());
    return;
}

auto Verifier::visit_if_stmt(std::shared_ptr<IfStmt> if_stmt) -> void {
    auto cond = if_stmt->get_cond();
    cond->visit(shared_from_this());
    if (!cond->get_type()->is_bool()) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type()->get_type_spec();
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
    if (!cond->get_type()->is_bool()) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type()->get_type_spec();
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

    check_duplicate_enums();
    for (auto& enum_ : current_module_->get_enums()) {
        enum_->visit(shared_from_this());
    }

    check_duplicate_globals();
    load_all_global_variables();
    for (auto& global_var : current_module_->get_global_vars()) {
        unmurk_decl(global_var);
        global_var->visit(shared_from_this());
    }

    check_duplicate_extern_declaration();
    for (auto& extern_ : current_module_->get_externs()) {
        extern_->visit(shared_from_this());
    }

    check_duplicate_function_declaration();
    for (auto& func : current_module_->get_functions()) {
        unmurk_decl(func);
        for (auto& para : func->get_paras()) {
            unmurk_decl(para);
            if (para->get_type()->is_array()) {
                auto a_t = std::dynamic_pointer_cast<ArrayType>(para->get_type());
                para->set_type(std::make_shared<PointerType>(a_t->get_sub_type()));
            }
        }
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
        for (auto const& enum_ : module->get_enums()) {
            if (!enum_->is_used()) {
                auto stream = "'" + enum_->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[41], stream, enum_->pos());
            }
        }
    }
}

auto Verifier::load_all_global_variables() -> void {
    for (auto const& global_var : current_module_->get_global_vars()) {
        global_var->set_statement_num(global_statement_counter_);
        global_var->set_depth_num(loop_depth_);
        declare_variable(global_var->get_ident() + global_var->get_append(), global_var);
    }
}

auto Verifier::check_duplicate_enums() -> void {
    std::vector<std::string> seen_enums;
    for (const auto& enum_ : current_module_->get_enums()) {
        auto it = std::find(seen_enums.begin(), seen_enums.end(), enum_->get_ident());
        if (it != seen_enums.end()) {
            handler_->report_error(current_filename_, all_errors_[36], enum_->get_ident(), enum_->pos());
        }
        else {
            seen_enums.push_back(enum_->get_ident());
        }
    }
}

auto Verifier::check_duplicate_globals() -> void {
    std::vector<GlobalVarDecl> seen_globals;
    for (const auto& global_var : current_module_->get_global_vars()) {
        auto it = std::find(seen_globals.begin(), seen_globals.end(), *global_var);
        if (it != seen_globals.end()) {
            handler_->report_error(current_filename_, all_errors_[30], global_var->get_ident(), global_var->pos());
        }
        else {
            seen_globals.push_back(*global_var);
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
    auto pos = ident.find('.');
    auto entry = symbol_table_.retrieve_one_level(ident.substr(0, pos));
    if (entry.has_value()) {
        const std::string error_message = "'" + ident.substr(0, pos) + "'. Previously declared at line "
                                          + std::to_string(entry->attr->pos().line_start_) + ", column "
                                          + std::to_string(entry->attr->pos().col_start_);
        if (auto derived_para_decl = std::dynamic_pointer_cast<ParaDecl>(decl)) {
            handler_->report_minor_error(current_filename_, all_errors_[3], error_message, decl->pos());
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

auto Verifier::unmurk_decl(std::shared_ptr<Decl> decl) -> void {
    unmurk_pos = decl->pos();
    auto new_t = unmurk(decl->get_type());
    decl->set_type(new_t);
}

auto Verifier::unmurk(std::shared_ptr<Type> murky_t) -> std::shared_ptr<Type> {
    if (murky_t->is_murky()) {
        return unmurk_direct(std::dynamic_pointer_cast<MurkyType>(murky_t));
    }
    else if (murky_t->is_array()) {
        auto l = std::dynamic_pointer_cast<ArrayType>(murky_t);
        if (l->get_length().has_value()) {
            return std::make_shared<ArrayType>(unmurk(l->get_sub_type()), *l->get_length());
        }
        else {
            return std::make_shared<ArrayType>(unmurk(l->get_sub_type()));
        }
    }
    else if (murky_t->is_pointer()) {
        auto l = std::dynamic_pointer_cast<PointerType>(murky_t);
        return std::make_shared<PointerType>(unmurk(l->get_sub_type()));
    }
    return murky_t;
}

auto Verifier::unmurk_direct(std::shared_ptr<MurkyType> murky_t) -> std::shared_ptr<Type> {
    auto lex = murky_t->get_name();

    for (auto& enum_ : current_module_->get_enums()) {
        if (enum_->get_ident() == lex) {
            return std::make_shared<EnumType>(enum_);
        }
    }

    handler_->report_error(current_filename_, all_errors_[42], lex, unmurk_pos);
    return std::make_shared<Type>(TypeSpec::ERROR);
}