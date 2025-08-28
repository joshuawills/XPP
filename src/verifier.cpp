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
    else if (local_var_decl->get_type()->is_array()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(local_var_decl->get_type());
        if (a_t->get_sub_type()->is_void()) {
            handler_->report_error(current_filename_, all_errors_[47], local_var_decl->get_ident(), local_var_decl->pos());
        }
    }

    if (local_var_decl->get_type()->is_numeric()) {
        current_numerical_type = local_var_decl->get_type();
    }
    local_var_decl->get_expr()->visit(shared_from_this());
    if (updated_expr_) {
        local_var_decl->set_expr(updated_expr_);
        updated_expr_ = nullptr;
    }

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
    else if (global_var_decl->get_type()->is_array()) {
        auto const a_t = std::dynamic_pointer_cast<ArrayType>(global_var_decl->get_type());
        if (a_t->get_sub_type()->is_void()) {
            handler_->report_error(current_filename_, all_errors_[47], name, global_var_decl->pos());
        }
    }

    if (global_var_decl->get_type()->is_numeric()) {
        current_numerical_type = global_var_decl->get_type();
    }
    global_var_decl->get_expr()->visit(shared_from_this());
    if (updated_expr_) {
        global_var_decl->set_expr(updated_expr_);
        updated_expr_ = nullptr;
    }
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

auto Verifier::visit_class_field_decl(std::shared_ptr<ClassFieldDecl> class_field_decl) -> void {
    auto const n = class_field_decl->get_ident();
    auto const t = class_field_decl->get_type();
    if (t->is_void()) {
        handler_->report_error(current_filename_, all_errors_[50], n, class_field_decl->pos());
    }
    else if (t->is_array()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(t);
        if (a_t->get_sub_type()->is_void()) {
            handler_->report_error(current_filename_, all_errors_[51], n, class_field_decl->pos());
        }
    }
}

auto Verifier::visit_class_decl(std::shared_ptr<ClassDecl> class_decl) -> void {
    curr_class = class_decl;
    auto const class_name = class_decl->get_ident();

    // Ensure there's no duplicate fields
    std::vector<std::string> seen_fields;
    for (auto const& field : class_decl->get_fields()) {
        field->visit(shared_from_this());
        auto n = field->get_ident();
        auto it = std::find(seen_fields.begin(), seen_fields.end(), n);
        if (it != seen_fields.end()) {
            auto err = "field '" + n + "' in class '" + class_name + "'";
            handler_->report_error(current_filename_, all_errors_[49], err, field->pos());
        }
        else {
            seen_fields.push_back(n);
        }
    }

    // Visit all the constructors
    check_duplicate_constructor_declaration(class_decl);
    for (auto& constructor : class_decl->get_constructors()) {
        constructor->visit(shared_from_this());
    }

    // Visit all the methods
    check_duplicate_method_declaration(class_decl);
    for (auto& method : class_decl->get_methods()) {
        method->visit(shared_from_this());
    }

    auto destructors = class_decl->get_destructors();
    if (destructors.size() > 1) {
        auto error = "class '" + class_name + "' has multiple destructors";
        handler_->report_error(current_filename_, all_errors_[80], error, destructors[1]->pos());
    }
    else {
        for (auto& destructor : destructors) {
            destructor->visit(shared_from_this());
        }
    }

    curr_class = nullptr;
    return;
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

auto Verifier::visit_constructor_decl(std::shared_ptr<ConstructorDecl> constructor_decl) -> void {
    global_statement_counter_ = 0;
    in_constructor_ = true;
    has_return_ = false;

    current_function_or_method_ = constructor_decl;
    symbol_table_.open_scope();
    // Add in the this keyword
    auto this_decl = std::make_shared<ParaDecl>(constructor_decl->pos(),
                                                "this",
                                                std::make_shared<PointerType>(curr_class->get_type()));
    this_decl->set_mut();
    this_decl->visit(shared_from_this());
    for (auto const& para : constructor_decl->get_paras()) {
        para->visit(shared_from_this());
    }
    constructor_decl->get_compound_stmt()->visit(shared_from_this());

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        // or if they were declared mutable but never reassigned
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used() and var.attr->get_ident() != "this") {
                auto err = "local variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], err, var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned() and var.attr->get_ident() != "this") {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    symbol_table_.close_scope();
    global_statement_counter_ = 0;
    has_return_ = false;
    in_constructor_ = false;
    return;
}

auto Verifier::visit_destructor_decl(std::shared_ptr<DestructorDecl> destructor_decl) -> void {
    in_destructor_ = true;
    current_function_or_method_ = destructor_decl;

    symbol_table_.open_scope();
    auto this_decl =
        std::make_shared<ParaDecl>(destructor_decl->pos(), "this", std::make_shared<PointerType>(curr_class->get_type()));
    this_decl->set_mut();
    this_decl->visit(shared_from_this());
    destructor_decl->get_compound_stmt()->visit(shared_from_this());
    symbol_table_.close_scope();

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        // or if they were declared mutable but never reassigned
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used() and var.attr->get_ident() != "this") {
                auto err = "local variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], err, var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned() and var.attr->get_ident() != "this") {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    global_statement_counter_ = 0;
    in_destructor_ = false;
}

auto Verifier::visit_method_decl(std::shared_ptr<MethodDecl> method_decl) -> void {
    global_statement_counter_ = 0;
    has_return_ = false;
    auto const m_type = method_decl->get_type();
    auto const m_name = method_decl->get_ident();
    auto const m_pos = method_decl->pos();

    if (m_type->is_array()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(m_type);
        if (a_t->get_sub_type()->is_void()) {
            handler_->report_error(current_filename_, all_errors_[47], "return type from method " + m_name, m_pos);
        }
        handler_->report_error(current_filename_, all_errors_[48], m_name, m_pos);
        method_decl->set_type(handler_->ERROR_TYPE);
        return;
    }

    current_function_or_method_ = method_decl;
    symbol_table_.open_scope();
    // Add in the this keyword
    auto this_decl =
        std::make_shared<ParaDecl>(method_decl->pos(), "this", std::make_shared<PointerType>(curr_class->get_type()));
    this_decl->set_mut();
    this_decl->visit(shared_from_this());
    for (auto const& para : method_decl->get_paras()) {
        para->visit(shared_from_this());
    }
    method_decl->get_compound_stmt()->visit(shared_from_this());

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        // or if they were declared mutable but never reassigned
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used() and var.attr->get_ident() != "this") {
                auto err = "local variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], err, var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned() and var.attr->get_ident() != "this") {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }
    symbol_table_.close_scope();

    if (!has_return_ and !m_type->is_void() and !m_type->is_error()) {
        handler_->report_error(current_filename_, all_errors_[10], "in method " + m_name, m_pos);
    }

    global_statement_counter_ = 0;
    return;
}

auto Verifier::visit_function(std::shared_ptr<Function> function) -> void {
    global_statement_counter_ = 0;
    has_return_ = false;
    auto func_t = function->get_type();

    if (func_t->is_array()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(func_t);
        if (a_t->get_sub_type()->is_void()) {
            handler_->report_error(current_filename_,
                                   all_errors_[47],
                                   "return type from function " + function->get_ident(),
                                   function->pos());
        }
        handler_->report_error(current_filename_, all_errors_[48], function->get_ident(), function->pos());
        function->set_type(handler_->ERROR_TYPE);
        return;
    }

    // Verifying main function
    if (function->get_ident() == "main") {
        in_main_ = has_main_ = true;
        if (!func_t->is_void()) {
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

    current_function_or_method_ = function;
    symbol_table_.open_scope();
    for (auto const& para : function->get_paras()) {
        para->visit(shared_from_this());
    }
    function->get_compound_stmt()->visit(shared_from_this());

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        // or if they were declared mutable but never reassigned
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used() and var.attr->get_ident() != "this") {
                auto err = "local variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], err, var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned() and var.attr->get_ident() != "this") {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    symbol_table_.close_scope();

    if (!has_return_ and !func_t->is_void() and !func_t->is_error()) {
        handler_->report_error(current_filename_, all_errors_[10], "in function " + function->get_ident(), function->pos());
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

    visiting_lhs_of_assignment_ = true;
    l->visit(shared_from_this());
    if (updated_expr_) {
        assignment_expr->set_lhs_expression(updated_expr_);
        l = updated_expr_;
        updated_expr_ = nullptr;
    }
    visiting_lhs_of_assignment_ = false;

    auto res = std::dynamic_pointer_cast<VarExpr>(l);
    auto deref_res = std::dynamic_pointer_cast<UnaryExpr>(l);
    auto array_index_res = std::dynamic_pointer_cast<ArrayIndexExpr>(l);
    auto class_field_res = std::dynamic_pointer_cast<FieldAccessExpr>(l);
    if ((!res and !deref_res and !array_index_res and !class_field_res)
        or (deref_res and deref_res->get_operator() != Op::DEREF))
    {
        handler_->report_error(current_filename_, all_errors_[7], "", assignment_expr->pos());
        assignment_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    if (res) {
        if (auto ref = res->get_ref()) {
            auto valid_constructor_mut = (in_constructor_ and std::dynamic_pointer_cast<ClassFieldDecl>(ref));
            if (!valid_constructor_mut) {
                ref->set_reassigned();
                if (!ref->is_mut()) {
                    handler_->report_error(current_filename_, all_errors_[20], res->get_name(), assignment_expr->pos());
                }
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
            auto valid_constructor_mut = (in_constructor_ and std::dynamic_pointer_cast<ClassFieldDecl>(ref));
            if (!valid_constructor_mut) {
                ref->set_reassigned();
                if (!ref->is_mut()) {
                    handler_->report_error(current_filename_, all_errors_[20], ref->get_ident(), assignment_expr->pos());
                }
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
            auto valid_constructor_mut = (in_constructor_ and std::dynamic_pointer_cast<ClassFieldDecl>(ref));
            if (!valid_constructor_mut) {
                ref->set_reassigned();
                if (!ref->is_mut()) {
                    handler_->report_error(current_filename_, all_errors_[20], ref->get_ident(), assignment_expr->pos());
                }
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
    if (updated_expr_) {
        assignment_expr->set_rhs_expression(updated_expr_);
        r = updated_expr_;
        updated_expr_ = nullptr;
    }

    current_numerical_type = std::nullopt;

    if (class_field_res) {
        auto ref = class_field_res->get_ref();
        ref->set_reassigned();
        if (!ref->is_mut()) {
            auto error = "field '" + class_field_res->get_field_name() + "' in class '"
                         + class_field_res->get_class_ref()->get_ident() + "' is marked constant";
            handler_->report_error(current_filename_, all_errors_[20], error, assignment_expr->pos());
        }
        if (l->get_type()->is_array()) {
            handler_->report_error(current_filename_, all_errors_[45], ref->get_ident(), assignment_expr->pos());
            assignment_expr->set_type(handler_->ERROR_TYPE);
            return;
        }

        if (auto l = std::dynamic_pointer_cast<VarExpr>(class_field_res->get_class_instance())) {
            l->get_ref()->set_reassigned();
            if (!l->get_ref()->is_mut()) {
                handler_->report_error(current_filename_, all_errors_[63], l->get_name(), assignment_expr->pos());
            }
        }
    }

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
    if (updated_expr_) {
        binary_expr->set_l_expr(updated_expr_);
        updated_expr_ = nullptr;
    }

    r->visit(shared_from_this());
    if (updated_expr_) {
        binary_expr->set_r_expr(updated_expr_);
        updated_expr_ = nullptr;
    }

    l = binary_expr->get_left();
    r = binary_expr->get_right();

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
    if (updated_expr_) {
        unary_expr->set_expr(updated_expr_);
        e = updated_expr_;
        updated_expr_ = nullptr;
    }
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
        auto valid_constructor_case = false;
        if (auto l = std::dynamic_pointer_cast<VarExpr>(e)) {
            valid_constructor_case = in_constructor_ and std::dynamic_pointer_cast<ClassFieldDecl>(l->get_ref());
            is_mut |= l->get_ref()->is_mut();
            is_lvalue = true;
            var_name = l->get_name();
        }
        if (auto l = std::dynamic_pointer_cast<UnaryExpr>(e)) {
            if (l->get_operator() == Op::DEREF) {
                auto v = std::dynamic_pointer_cast<VarExpr>(l->get_expr());
                valid_constructor_case = in_constructor_ and std::dynamic_pointer_cast<ClassFieldDecl>(v->get_ref());
                is_mut |= v->get_ref()->is_mut();
                is_lvalue = true;
                var_name = v->get_name();
            }
        }
        if (!is_lvalue) {
            handler_->report_error(current_filename_, all_errors_[28], "", unary_expr->pos());
            unary_expr->set_type(handler_->ERROR_TYPE);
        }
        else if (!is_mut and !valid_constructor_case) {
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

auto Verifier::visit_null_expr(std::shared_ptr<NullExpr> null_expr) -> void {
    (void)null_expr;
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
    auto n = var_expr->get_name();
    std::shared_ptr<Decl> d;
    auto entry = symbol_table_.retrieve(n);

    if (!entry.has_value() and curr_module_access_) {
        // Checking for global var
        auto found = false;
        for (auto& global_var : curr_module_access_->get_global_vars()) {
            if (global_var->get_ident() == n) {
                d = global_var;
                if (!d->is_pub()) {
                    handler_->report_error(current_filename_,
                                           all_errors_[77],
                                           "variable '" + n + "' is marked private",
                                           var_expr->pos());
                    var_expr->set_type(handler_->ERROR_TYPE);
                    return;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            auto error = "global variable '" + n + "' not found in module '" + curr_module_alias_ + "'";
            handler_->report_error(current_filename_, all_errors_[78], error, var_expr->pos());
            var_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
    }
    else if (!entry.has_value()) {
        auto const method_d = std::dynamic_pointer_cast<MethodDecl>(current_function_or_method_);
        auto const constructor_d = std::dynamic_pointer_cast<ConstructorDecl>(current_function_or_method_);
        auto const destructor_d = std::dynamic_pointer_cast<DestructorDecl>(current_function_or_method_);
        if (!method_d and !constructor_d and !destructor_d) {
            handler_->report_error(current_filename_, all_errors_[8], n, var_expr->pos());
            var_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
        auto found = false;
        for (auto& para : curr_class->get_fields()) {
            if (para->get_ident() == n) {
                if (visiting_lhs_of_assignment_ and method_d and !method_d->is_mut()) {
                    auto error = "field '" + n + "' can't be mutated in constant method '" + method_d->get_ident() + "'";
                    handler_->report_error(current_filename_, all_errors_[69], error, var_expr->pos());
                }
                d = para;
                found = true;
                break;
            }
        }

        if (!found) {
            handler_->report_error(current_filename_, all_errors_[8], n, var_expr->pos());
            var_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
    }
    else {
        d = entry->attr;
    }
    var_expr->set_ref(d);
    var_expr->set_type(d->get_type());
    var_expr->get_ref()->set_used();
    return;
}

auto Verifier::visit_call_expr(std::shared_ptr<CallExpr> call_expr) -> void {
    auto const function_name = call_expr->get_name();

    if (curr_module_access_ and curr_module_access_->class_with_name_exists(function_name)) {
        auto constructor_call_expr =
            std::make_shared<ConstructorCallExpr>(call_expr->pos(), function_name, call_expr->get_args());
        constructor_call_expr->visit(shared_from_this());
        updated_expr_ = constructor_call_expr;
        return;
    }

    if (current_module_->class_with_name_exists(function_name)) {
        auto constructor_call_expr =
            std::make_shared<ConstructorCallExpr>(call_expr->pos(), function_name, call_expr->get_args());
        constructor_call_expr->visit(shared_from_this());
        updated_expr_ = constructor_call_expr;
        return;
    }

    if (!current_module_->function_with_name_exists(function_name) and !curr_module_access_) {
        handler_->report_error(current_filename_, all_errors_[12], function_name, call_expr->pos());
        return;
    }
    if (in_main_ and function_name == "main") {
        handler_->report_error(current_filename_, all_errors_[13], "", call_expr->pos());
        return;
    }

    auto updated_args = std::vector<std::shared_ptr<Expr>>{};
    for (auto& arg : call_expr->get_args()) {
        arg->visit(shared_from_this());
        if (updated_expr_) {
            updated_args.push_back(updated_expr_);
            updated_expr_ = nullptr;
        }
        else {
            updated_args.push_back(arg);
        }
    }
    call_expr->set_args(updated_args);

    std::optional<std::shared_ptr<Decl>> equivalent_func;
    if (curr_module_access_) {
        equivalent_func = curr_module_access_->get_decl(call_expr);
        if (!equivalent_func) {
            auto error = "function '" + function_name + "' not found in module '" + curr_module_alias_ + "'";
            handler_->report_error(current_filename_, all_errors_[14], error, call_expr->pos());
            call_expr->set_type(handler_->ERROR_TYPE);
            return;
        }

        auto is_extern = std::dynamic_pointer_cast<Extern>(*equivalent_func);
        if (!(*equivalent_func)->is_pub()) {
            auto error = std::string{};
            if (is_extern) {
                error += "extern";
            }
            else {
                error += "function";
            }
            error += " '" + function_name + "' is private";
            handler_->report_error(current_filename_, all_errors_[74], error, call_expr->pos());
            call_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
    }
    else {
        equivalent_func = current_module_->get_decl(call_expr);
        if (!equivalent_func) {
            handler_->report_error(current_filename_, all_errors_[14], function_name, call_expr->pos());
            call_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
    }

    (*equivalent_func)->set_used();
    call_expr->set_ref(*equivalent_func);
    call_expr->set_type((*equivalent_func)->get_type());
    return;
}

auto Verifier::visit_constructor_call_expr(std::shared_ptr<ConstructorCallExpr> constructor_call_expr) -> void {
    auto p = constructor_call_expr->pos();
    for (auto& arg : constructor_call_expr->get_args()) {
        arg->visit(shared_from_this());
    }

    std::optional<std::shared_ptr<ConstructorDecl>> equivalent_constructor;
    if (curr_module_access_) {
        equivalent_constructor = curr_module_access_->get_constructor_decl(constructor_call_expr);
    }
    else {
        equivalent_constructor = current_module_->get_constructor_decl(constructor_call_expr);
    }
    if (!equivalent_constructor) {
        handler_->report_error(current_filename_,
                               all_errors_[59],
                               "on class: '" + constructor_call_expr->get_name() + "'",
                               p);
        constructor_call_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    if (!(*equivalent_constructor)->is_pub() and !in_constructor_) {
        handler_->report_error(current_filename_, all_errors_[76], "", constructor_call_expr->pos());
    }

    auto paras = (*equivalent_constructor)->get_paras();
    auto c = 0u;
    for (auto& arg : constructor_call_expr->get_args()) {
        if (c < paras.size() and paras[c]->get_type()->is_numeric()) {
            current_numerical_type = paras[c]->get_type();
        }
        arg->visit(shared_from_this());
        current_numerical_type = std::nullopt;
        ++c;
    }

    (*equivalent_constructor)->set_used();
    auto class_ref = std::dynamic_pointer_cast<ClassType>((*equivalent_constructor)->get_type());
    if (class_ref) {
        if (curr_module_access_ and !class_ref->get_ref()->is_pub()) {
            auto error = "class '" + class_ref->get_ref()->get_ident() + "' is not accessible outside of its module";
            handler_->report_error(current_filename_, all_errors_[75], error, constructor_call_expr->pos());
        }
        class_ref->get_ref()->set_used();
    }
    else {
        std::cout << "unreachable emitter::visit_constructor_call_expr";
    }
    constructor_call_expr->set_ref(*equivalent_constructor);
    constructor_call_expr->set_type((*equivalent_constructor)->get_type());
    return;
}

auto Verifier::visit_cast_expr(std::shared_ptr<CastExpr> cast_expr) -> void {
    auto expr = cast_expr->get_expr();
    if (updated_expr_) {
        cast_expr->set_expr(updated_expr_);
        expr = updated_expr_;
        updated_expr_ = nullptr;
    }
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
    auto updated_exprs = std::vector<std::shared_ptr<Expr>>{};
    for (auto& expr : array_init_expr->get_exprs()) {
        expr->visit(shared_from_this());
        if (updated_expr_) {
            updated_exprs.push_back(expr);
            updated_expr_ = nullptr;
        }
        else {
            updated_exprs.push_back(expr);
        }

        auto last_expr = updated_exprs.back();
        auto last_t = last_expr->get_type();

        if (elem_num == 0 and !has_sub_type_specified) {
            element_types = last_t;
        }

        if (!last_t->is_error() and *last_t != *element_types) {
            auto stream = std::stringstream{};
            stream << "position " << elem_num << ". Expected " << *element_types << ", got " << *last_t;
            if (element_types->is_numeric() and last_t->is_numeric()) {
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

    // Update the args vector
    array_init_expr->set_exprs(updated_exprs);

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
    std::optional<std::shared_ptr<EnumDecl>> ref;
    if (curr_module_access_) {
        ref = curr_module_access_->get_enum(enum_name);
    }
    else {
        ref = current_module_->get_enum(enum_name);
    }
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

auto Verifier::visit_field_access_expr(std::shared_ptr<FieldAccessExpr> field_access_expr) -> void {
    field_access_expr->get_class_instance()->visit(shared_from_this());
    if (updated_expr_) {
        field_access_expr->set_class_instance(updated_expr_);
        updated_expr_ = nullptr;
    }

    auto is_this = false;
    if (auto l = std::dynamic_pointer_cast<VarExpr>(field_access_expr->get_class_instance())) {
        is_this = l->get_name() == "this";
    }

    std::shared_ptr<ClassType> class_type;
    if (field_access_expr->is_arrow()) {
        if (!field_access_expr->get_class_instance()->get_type()->is_pointer()) {
            auto error = std::stringstream{};
            error << "received type " << *field_access_expr->get_class_instance()->get_type()
                  << " instead of a pointer type";
            handler_->report_error(current_filename_, all_errors_[79], error.str(), field_access_expr->pos());
            field_access_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
        auto pointer_type = std::dynamic_pointer_cast<PointerType>(field_access_expr->get_class_instance()->get_type());
        class_type = std::dynamic_pointer_cast<ClassType>(pointer_type->get_sub_type());
    }
    else {
        class_type = std::dynamic_pointer_cast<ClassType>(field_access_expr->get_class_instance()->get_type());
    }

    if (!class_type) {
        auto error = std::stringstream{};
        error << "received type " << *field_access_expr->get_class_instance()->get_type();
        handler_->report_error(current_filename_, all_errors_[60], error.str(), field_access_expr->pos());
        field_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    auto class_ref = class_type->get_ref();
    assert(class_ref != nullptr);

    auto f_name = field_access_expr->get_field_name();
    if (!class_ref->field_exists(f_name)) {
        auto error = "field '" + f_name + "' does not exist on class '" + class_ref->get_ident() + "'";
        handler_->report_error(current_filename_, all_errors_[61], error, field_access_expr->pos());
        field_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    auto ref = class_ref->get_field(f_name);

    if (!is_this and !ref->is_pub()) {
        auto error = "field '" + f_name + "' is marked private in class '" + class_ref->get_ident() + "'";
        handler_->report_error(current_filename_, all_errors_[62], error, field_access_expr->pos());
        field_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }
    ref->set_used();

    field_access_expr->set_class_ref(class_ref);
    field_access_expr->set_ref(ref);
    field_access_expr->set_field_num(class_ref->get_index_for_field(f_name));
    field_access_expr->set_type(class_ref->get_field_type(f_name));
    return;
}

auto Verifier::visit_method_access_expr(std::shared_ptr<MethodAccessExpr> method_access_expr) -> void {
    auto const n = method_access_expr->get_method_name();
    method_access_expr->get_class_instance()->visit(shared_from_this());
    if (updated_expr_) {
        method_access_expr->set_class_instance(updated_expr_);
        updated_expr_ = nullptr;
    }

    std::shared_ptr<ClassType> class_type;
    if (method_access_expr->is_arrow()) {
        if (!method_access_expr->get_class_instance()->get_type()->is_pointer()) {
            auto error = std::stringstream{};
            error << "received type " << *method_access_expr->get_class_instance()->get_type()
                  << " instead of a pointer type";
            handler_->report_error(current_filename_, all_errors_[79], error.str(), method_access_expr->pos());
            method_access_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
        auto pointer_type = std::dynamic_pointer_cast<PointerType>(method_access_expr->get_class_instance()->get_type());
        class_type = std::dynamic_pointer_cast<ClassType>(pointer_type->get_sub_type());
    }
    else {
        class_type = std::dynamic_pointer_cast<ClassType>(method_access_expr->get_class_instance()->get_type());
    }

    if (!class_type) {
        auto error = std::stringstream{};
        error << "received type " << *method_access_expr->get_class_instance()->get_type();
        handler_->report_error(current_filename_, all_errors_[64], error.str(), method_access_expr->pos());
        method_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    auto class_ref = class_type->get_ref();

    if (!class_ref->method_exists(n)) {
        auto error = "method '" + n + "' does not exist on class '" + class_ref->get_ident() + "'";
        handler_->report_error(current_filename_, all_errors_[65], error, method_access_expr->pos());
        method_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    for (auto& arg : method_access_expr->get_args()) {
        arg->visit(shared_from_this());
    }

    auto method_ref = class_ref->get_method(method_access_expr);
    if (!method_ref) {
        auto error = "method '" + n + "' on class '" + class_ref->get_ident() + "' does not match provided parameters";
        handler_->report_error(current_filename_, all_errors_[66], error, method_access_expr->pos());
        method_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    if (!(*method_ref)->is_pub()) {
        auto error = "method '" + n + "' is not accessible on class '" + class_ref->get_ident() + "'";
        handler_->report_error(current_filename_, all_errors_[67], error, method_access_expr->pos());
        method_access_expr->set_type(handler_->ERROR_TYPE);
        return;
    }

    auto paras = (*method_ref)->get_paras();
    auto c = 0u;
    auto new_args = std::vector<std::shared_ptr<Expr>>{};
    for (auto& arg : method_access_expr->get_args()) {
        if (paras[c]->get_type()->is_numeric()) {
            current_numerical_type = paras[c]->get_type();
        }
        arg->visit(shared_from_this());
        if (updated_expr_) {
            new_args.push_back(updated_expr_);
            updated_expr_ = nullptr;
        }
        else {
            new_args.push_back(arg);
        }
        current_numerical_type = std::nullopt;
        c++;
    }
    method_access_expr->set_args(new_args);
    (*method_ref)->set_used();
    method_access_expr->set_ref(*method_ref);
    method_access_expr->set_type((*method_ref)->get_type());

    if ((*method_ref)->is_mut()) {
        if (auto v = std::dynamic_pointer_cast<VarExpr>(method_access_expr->get_class_instance())) {
            if (!v->get_ref()->is_mut()) {
                auto error = "mutable method '" + (*method_ref)->get_ident() + "' called on a non-mutable variable '"
                             + v->get_name() + "'";
                handler_->report_error(current_filename_, all_errors_[68], error, method_access_expr->pos());
            }
            v->get_ref()->set_reassigned();
        }
    }

    return;
}

auto Verifier::visit_size_of_expr(std::shared_ptr<SizeOfExpr> size_of_expr) -> void {
    if (size_of_expr->is_type()) {
        if (size_of_expr->get_type_to_size()->is_murky()) {
            auto m_t = std::dynamic_pointer_cast<MurkyType>(size_of_expr->get_type_to_size());
            size_of_expr->set_type_to_size(unmurk_direct(m_t));
        }
    }
    else {
        size_of_expr->get_expr_to_size()->visit(shared_from_this());
        if (updated_expr_) {
            size_of_expr->set_expr_to_size(updated_expr_);
            updated_expr_ = nullptr;
        }
    }
    return;
}

auto Verifier::visit_import_expr(std::shared_ptr<ImportExpr> import_expr) -> void {
    auto alias_s = import_expr->get_alias_name();
    // Check if alias exists
    auto module = current_module_->get_module_from_alias(alias_s);

    if (!module) {
        // Check if an enum exists with that name
        std::optional<std::shared_ptr<EnumDecl>> potential_enum;
        if (curr_module_access_) {
            potential_enum = curr_module_access_->get_enum(alias_s);
        }
        else {
            potential_enum = current_module_->get_enum(alias_s);
        }

        auto is_var_expr = std::dynamic_pointer_cast<VarExpr>(import_expr->get_expr());
        if (potential_enum.has_value() and is_var_expr) {
            auto enum_access_expr = std::make_shared<EnumAccessExpr>(import_expr->pos(),
                                                                     potential_enum.value()->get_ident(),
                                                                     is_var_expr->get_name());
            enum_access_expr->visit(shared_from_this());
            if (updated_expr_) {
                import_expr->set_expr(updated_expr_);
            }
            else {
                updated_expr_ = enum_access_expr;
            }
        }
        else {
            auto error = "module '" + alias_s + "'";
            handler_->report_error(current_filename_, all_errors_[38], error, import_expr->pos());
            import_expr->set_type(handler_->ERROR_TYPE);
        }
        return;
    }

    curr_module_access_ = *module;
    curr_module_alias_ = alias_s;
    import_expr->set_module_ref(*module);
    import_expr->get_expr()->visit(shared_from_this());
    if (updated_expr_) {
        import_expr->set_expr(updated_expr_);
    }
    curr_module_access_ = nullptr;

    updated_expr_ = import_expr->get_expr();

    return;
}

auto Verifier::visit_new_expr(std::shared_ptr<NewExpr> new_expr) -> void {
    if (new_expr->get_new_type()->is_murky()) {
        new_expr->set_new_type(unmurk_direct(std::dynamic_pointer_cast<MurkyType>(new_expr->get_new_type())));
    }
    auto t = new_expr->get_new_type();

    auto array_arg = new_expr->get_array_size_args();
    auto constructor_arg = new_expr->get_constructor_args();
    if (!array_arg.has_value() and !constructor_arg.has_value()) {
        if (t->is_void()) {
            handler_->report_error(current_filename_, all_errors_[82], "", new_expr->pos());
            new_expr->set_type(handler_->ERROR_TYPE);
            return;
        }
    }
    else if (array_arg.has_value()) {
        // We have an array allocation
        if (t->is_void()) {
            handler_->report_error(current_filename_, all_errors_[82], "", new_expr->pos());
            new_expr->set_type(handler_->ERROR_TYPE);
            return;
        }

        array_arg.value()->visit(shared_from_this());
        if (updated_expr_) {
            new_expr->set_array_size_args(updated_expr_);
            array_arg = updated_expr_;
            updated_expr_ = nullptr;
        }

        if (!(*array_arg)->get_type()->is_i64()) {
            auto error = std::stringstream{};
            error << "received " << *(*array_arg)->get_type();
            handler_->report_error(current_filename_, all_errors_[83], error.str(), (*array_arg)->pos());
        }
    }
    else if (constructor_arg.has_value()) {
        // We have a call to a class constructor
        // Check for import type
        std::shared_ptr<ClassType> class_type = nullptr;
        std::shared_ptr<Module> curr_module_access = nullptr;
        std::string name = {};
        if (t->is_class()) {
            class_type = std::dynamic_pointer_cast<ClassType>(t);
            name = class_type->get_ref()->get_ident();
        }
        else if (t->is_import()) {
            auto mod = current_module_->get_module_from_alias(std::dynamic_pointer_cast<ImportType>(t)->get_name());
            t = unmurk(t);
            if (t->is_class()) {
                class_type = std::dynamic_pointer_cast<ClassType>(t);
                name = class_type->get_ref()->get_ident();
            }
            curr_module_access = *mod;
        }

        if (!class_type) {
            auto error = std::stringstream{};
            error << "received " << *t;
            handler_->report_error(current_filename_, all_errors_[84], error.str(), new_expr->pos());
            new_expr->set_type(handler_->ERROR_TYPE);
            return;
        }

        auto new_constructor_args = std::vector<std::shared_ptr<Expr>>{};
        for (auto& arg : *constructor_arg) {
            arg->visit(shared_from_this());
            if (updated_expr_) {
                new_constructor_args.push_back(updated_expr_);
                updated_expr_ = nullptr;
            }
            else {
                new_constructor_args.push_back(arg);
            }
        }
        *constructor_arg = new_constructor_args;

        auto constructor_call_expr = std::make_shared<ConstructorCallExpr>(new_expr->pos(), name, *constructor_arg);

        std::optional<std::shared_ptr<ConstructorDecl>> equivalent_constructor;
        if (curr_module_access) {
            equivalent_constructor = curr_module_access->get_constructor_decl(constructor_call_expr);
        }
        else {
            equivalent_constructor = current_module_->get_constructor_decl(constructor_call_expr);
        }

        if (!equivalent_constructor) {
            handler_->report_error(current_filename_,
                                   all_errors_[59],
                                   "on class: '" + constructor_call_expr->get_name() + "'",
                                   new_expr->pos());
            new_expr->set_type(handler_->ERROR_TYPE);
            return;
        }

        if (!(*equivalent_constructor)->is_pub()) {
            handler_->report_error(current_filename_, all_errors_[76], "", constructor_call_expr->pos());
        }

        auto paras = (*equivalent_constructor)->get_paras();
        auto c = 0u;
        for (auto& arg : constructor_call_expr->get_args()) {
            if (c < paras.size() and paras[c]->get_type()->is_numeric()) {
                current_numerical_type = paras[c]->get_type();
            }
            arg->visit(shared_from_this());
            current_numerical_type = std::nullopt;
            ++c;
        }

        (*equivalent_constructor)->set_used();
        auto class_ref = std::dynamic_pointer_cast<ClassType>((*equivalent_constructor)->get_type());
        if (class_ref) {
            if (curr_module_access_ and !class_ref->get_ref()->is_pub()) {
                auto error = "class '" + class_ref->get_ref()->get_ident() + "' is not accessible outside of its module";
                handler_->report_error(current_filename_, all_errors_[75], error, constructor_call_expr->pos());
                constructor_call_expr->set_type(handler_->ERROR_TYPE);
            }
            else {
                class_ref->get_ref()->set_used();
            }
        }
        else {
            std::cout << "unreachable emitter::visit_constructor_call_expr";
        }
        constructor_call_expr->set_ref(*equivalent_constructor);
        constructor_call_expr->set_type((*equivalent_constructor)->get_type());

        new_expr->set_call_expr(constructor_call_expr);
    }

    new_expr->set_type(std::make_shared<PointerType>(t));
    return;
}

auto Verifier::visit_empty_stmt(std::shared_ptr<EmptyStmt> empty_stmt) -> void {
    (void)empty_stmt;
    return;
}

auto Verifier::visit_compound_stmt(std::shared_ptr<CompoundStmt> compound_stmt) -> void {
    auto p = compound_stmt->get_parent();

    symbol_table_.open_scope();
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

    if (!handler_->quiet_mode()) {
        // Check if any variables opened in that scope remained unused
        for (auto const& var : symbol_table_.retrieve_latest_scope()) {
            if (!var.attr->is_used() and var.attr->get_ident() != "this") {
                auto stream = std::stringstream{};
                stream << "local variable '" << var.attr->get_ident() << "'";
                handler_->report_minor_error(current_filename_, all_errors_[21], stream.str(), var.attr->pos());
            }
            if (var.attr->is_mut() and !var.attr->is_reassigned() and var.attr->get_ident() != "this") {
                auto err = "variable '" + var.attr->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[44], err, var.attr->pos());
            }
        }
    }

    auto latest_scope = symbol_table_.retrieve_latest_scope();
    for (auto it = latest_scope.rbegin(); it != latest_scope.rend(); ++it) {
        auto member = *it;
        if (member.attr->get_type()->is_class()) {
            auto expr =
                std::make_shared<VarExpr>(compound_stmt->pos(), member.attr->get_ident(), member.attr->get_type());
            auto delete_stmt = std::make_shared<DeleteStmt>(compound_stmt->pos(), expr);
            delete_stmt->visit(shared_from_this());
            compound_stmt->add_stmt(delete_stmt);
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
    if (updated_expr_) {
        return_stmt->set_expr(updated_expr_);
        expr = updated_expr_;
        updated_expr_ = nullptr;
    }
    auto is_constructor = std::dynamic_pointer_cast<ConstructorDecl>(current_function_or_method_);

    if (is_constructor) {
        auto is_empty_expr = std::dynamic_pointer_cast<EmptyExpr>(expr);
        if (!is_empty_expr) {
            auto error = "in class '" + curr_class->get_ident() + "'";
            handler_->report_error(current_filename_, all_errors_[57], error, expr->pos());
        }
        return;
    }

    if (current_function_or_method_->get_type()->is_numeric()) {
        current_numerical_type = current_function_or_method_->get_type();
    }
    expr->visit(shared_from_this());
    current_numerical_type = std::nullopt;

    auto const expr_type = expr->get_type();
    if (*expr_type != *current_function_or_method_->get_type()) {
        auto stream = std::stringstream{};
        stream << "in function " << current_function_or_method_->get_ident() << ". expected type "
               << *current_function_or_method_->get_type() << ", received " << *expr_type;
        handler_->report_error(current_filename_, all_errors_[11], stream.str(), return_stmt->pos());
    }

    return;
}

auto Verifier::visit_expr_stmt(std::shared_ptr<ExprStmt> expr_stmt) -> void {
    expr_stmt->get_expr()->visit(shared_from_this());
    if (updated_expr_) {
        expr_stmt->set_expr(updated_expr_);
        updated_expr_ = nullptr;
    }
}

auto Verifier::visit_while_stmt(std::shared_ptr<WhileStmt> while_stmt) -> void {
    auto cond = while_stmt->get_cond();
    cond->visit(shared_from_this());

    if (!cond->get_type()->is_bool()) {
        auto stream = std::stringstream{};
        stream << "received " << cond->get_type()->get_type_spec();
        handler_->report_error(current_filename_, all_errors_[19], stream.str(), cond->pos());
    }

    loop_depth_++;
    while_stmt->get_stmts()->visit(shared_from_this());
    loop_depth_--;
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

auto Verifier::visit_loop_stmt(std::shared_ptr<LoopStmt> loop_stmt) -> void {
    symbol_table_.open_scope();

    auto var = std::make_shared<LocalVarDecl>(loop_stmt->pos(),
                                              loop_stmt->get_var_name(),
                                              std::make_shared<Type>(TypeSpec::I64),
                                              std::make_shared<EmptyExpr>(loop_stmt->pos()));
    var->set_statement_num(global_statement_counter_);
    var->set_depth_num(loop_depth_);
    loop_stmt->set_var_decl(var);
    declare_variable(loop_stmt->get_var_name(), var);

    if (loop_stmt->has_lower_bound()) {
        auto lower_bound = *loop_stmt->get_lower_bound();
        lower_bound->visit(shared_from_this());
        if (updated_expr_) {
            loop_stmt->set_lower_bound(updated_expr_);
            updated_expr_ = nullptr;
        }
        lower_bound = *loop_stmt->get_lower_bound();

        auto lower_t = lower_bound->get_type();
        if (!lower_t->is_i64()) {
            handler_->report_error(current_filename_,
                                   all_errors_[70],
                                   "received type " + lower_t->to_string(),
                                   lower_bound->pos());
            return;
        }
    }

    if (loop_stmt->has_upper_bound()) {
        auto upper_bound = *loop_stmt->get_upper_bound();
        upper_bound->visit(shared_from_this());
        if (updated_expr_) {
            loop_stmt->set_upper_bound(updated_expr_);
            updated_expr_ = nullptr;
        }
        upper_bound = *loop_stmt->get_upper_bound();

        auto upper_t = upper_bound->get_type();
        if (!upper_t->is_i64()) {
            handler_->report_error(current_filename_,
                                   all_errors_[71],
                                   "received type " + upper_t->to_string(),
                                   upper_bound->pos());
            return;
        }
    }
    loop_depth_++;
    loop_stmt->get_body_stmt()->visit(shared_from_this());
    loop_depth_--;

    symbol_table_.close_scope();
    return;
}

auto Verifier::visit_break_stmt(std::shared_ptr<BreakStmt> break_stmt) -> void {
    if (loop_depth_ <= 0) {
        handler_->report_error(current_filename_, all_errors_[72], "", break_stmt->pos());
    }
    return;
}

auto Verifier::visit_continue_stmt(std::shared_ptr<ContinueStmt> continue_stmt) -> void {
    if (loop_depth_ <= 0) {
        handler_->report_error(current_filename_, all_errors_[73], "", continue_stmt->pos());
    }
    return;
}

auto Verifier::visit_delete_stmt(std::shared_ptr<DeleteStmt> delete_stmt) -> void {
    delete_stmt->get_expr()->visit(shared_from_this());
    if (updated_expr_) {
        delete_stmt->set_expr(updated_expr_);
        updated_expr_ = nullptr;
    }
    auto const t = delete_stmt->get_expr()->get_type();
    if (!t->is_pointer() and !t->is_class()) {
        auto error = "received type " + t->to_string();
        handler_->report_error(current_filename_, all_errors_[81], error, delete_stmt->pos());
    }
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

    // Load in all imported modules
    for (auto const& import : current_module_->get_imported_filepaths()) {
        auto imported_module = modules_->get_module_from_filepath(import);
        if (imported_module) {
            current_module_->add_imported_module(import, imported_module);
        }
        else {
            // Time to do all the work with it
            auto new_verifier = std::make_shared<Verifier>(handler_, modules_);
            new_verifier->check(import, false);
            current_module_->add_imported_module(import, modules_->get_module_from_filepath(import));
        }
    }

    check_duplicate_custom_type();
    for (auto& enum_ : current_module_->get_enums()) {
        enum_->visit(shared_from_this());
    }
    for (auto& class_ : current_module_->get_classes()) {
        for (auto& field : class_->get_fields()) {
            unmurk_decl(field);
        }
        for (auto& method : class_->get_methods()) {
            unmurk_decl(method);
            for (auto& para : method->get_paras()) {
                unmurk_decl(para);
                if (para->get_type()->is_array()) {
                    auto a_t = std::dynamic_pointer_cast<ArrayType>(para->get_type());
                    if (a_t->get_sub_type()->is_void()) {
                        handler_->report_error(current_filename_, all_errors_[47], para->get_ident(), para->pos());
                        para->set_type(handler_->ERROR_TYPE);
                    }
                    else {
                        para->set_type(std::make_shared<PointerType>(a_t->get_sub_type()));
                    }
                }
            }
        }
        for (auto& constructor : class_->get_constructors()) {
            constructor->set_type(std::make_shared<ClassType>(class_));
            for (auto& para : constructor->get_paras()) {
                unmurk_decl(para);
                if (para->get_type()->is_array()) {
                    auto a_t = std::dynamic_pointer_cast<ArrayType>(para->get_type());
                    if (a_t->get_sub_type()->is_void()) {
                        handler_->report_error(current_filename_, all_errors_[47], para->get_ident(), para->pos());
                        para->set_type(handler_->ERROR_TYPE);
                    }
                    else {
                        para->set_type(std::make_shared<PointerType>(a_t->get_sub_type()));
                    }
                }
            }
        }
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
                if (a_t->get_sub_type()->is_void()) {
                    handler_->report_error(current_filename_, all_errors_[47], para->get_ident(), para->pos());
                    para->set_type(handler_->ERROR_TYPE);
                }
                else {
                    para->set_type(std::make_shared<PointerType>(a_t->get_sub_type()));
                }
            }
        }
        func->visit(shared_from_this());
    }

    for (auto& class_ : current_module_->get_classes()) {
        class_->visit(shared_from_this());
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
        for (auto const& class_ : module->get_classes()) {
            if (!class_->is_used()) {
                auto stream = "'" + class_->get_ident() + "'";
                handler_->report_minor_error(current_filename_, all_errors_[52], stream, class_->pos());
            }
            for (auto const& constructor : class_->get_constructors()) {
                if (!constructor->is_used()) {
                    auto stream = "in class '" + class_->get_ident() + "' at line "
                                  + std::to_string(constructor->pos().line_start_);
                    handler_->report_minor_error(current_filename_, all_errors_[55], stream, constructor->pos());
                }
            }
            for (auto const& method : class_->get_methods()) {
                if (!method->is_used()) {
                    auto stream = std::string{};
                    if (method->is_pub()) {
                        stream += "public ";
                    }
                    else {
                        stream += "private ";
                    }
                    stream += "method '" + method->get_ident() + "' in class '" + class_->get_ident() + "'";
                    handler_->report_minor_error(current_filename_, all_errors_[53], stream, method->pos());
                }
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

auto Verifier::check_duplicate_custom_type() -> void {
    std::unordered_map<std::string, Position> enums_seen;
    std::unordered_map<std::string, Position> classes_seen;

    for (auto const& enum_ : current_module_->get_enums()) {
        auto const& name = enum_->get_ident();
        auto const& pos = enum_->pos();

        if (auto it = enums_seen.find(name); it != enums_seen.end()) {
            handler_->report_error(
                current_filename_,
                all_errors_[36],
                "enum '" + name + "' previously defined as an enum at line " + std::to_string(it->second.line_start_),
                pos);
        }
        else {
            enums_seen[name] = pos;
        }
    }

    for (auto const& class_ : current_module_->get_classes()) {
        auto const& name = class_->get_ident();
        auto const& pos = class_->pos();

        if (auto it_enum = enums_seen.find(name); it_enum != enums_seen.end()) {
            handler_->report_error(current_filename_,
                                   all_errors_[36],
                                   "class '" + name + "' previously defined as an enum at line "
                                       + std::to_string(it_enum->second.line_start_),
                                   pos);
        }
        else if (auto it_class = classes_seen.find(name); it_class != classes_seen.end()) {
            handler_->report_error(current_filename_,
                                   all_errors_[36],
                                   "class '" + name + "' previously defined as a class at line "
                                       + std::to_string(it_class->second.line_start_),
                                   pos);
        }
        else {
            classes_seen[name] = pos;
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
        if (current_module_->class_with_name_exists(func->get_ident())) {
            handler_->report_error(current_filename_,
                                   all_errors_[58],
                                   "function '" + func->get_ident() + "' conflicts with class",
                                   func->pos());
        }

        auto it = std::find(seen_functions.begin(), seen_functions.end(), *func);
        if (it != seen_functions.end()) {
            handler_->report_error(current_filename_, all_errors_[1], func->get_ident(), func->pos());
        }
        else {
            seen_functions.push_back(*func);
        }
    }
}

auto Verifier::check_duplicate_method_declaration(std::shared_ptr<ClassDecl>& class_decl) -> void {
    std::vector<MethodDecl> seen_methods;
    for (auto const& method : class_decl->get_methods()) {
        auto it = std::find(seen_methods.begin(), seen_methods.end(), *method);
        if (it != seen_methods.end()) {
            auto error = "method '" + method->get_ident() + "' on class '" + curr_class->get_ident() + "'";
            handler_->report_error(current_filename_, all_errors_[54], error, method->pos());
        }
        else {
            seen_methods.push_back(*method);
        }
    }
}

auto Verifier::check_duplicate_constructor_declaration(std::shared_ptr<ClassDecl>& class_decl) -> void {
    std::vector<ConstructorDecl> seen_constructors;
    for (auto const& constructor : class_decl->get_constructors()) {
        auto it = std::find(seen_constructors.begin(), seen_constructors.end(), *constructor);
        if (it != seen_constructors.end()) {
            auto const error = "on class '" + curr_class->get_ident() + "' previously declared at line "
                               + std::to_string(it->pos().line_start_);
            handler_->report_error(current_filename_, all_errors_[56], error, constructor->pos());
        }
        else {
            seen_constructors.push_back(*constructor);
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
    else if (murky_t->is_import()) {
        auto l = std::dynamic_pointer_cast<ImportType>(murky_t);
        assert(l->get_sub_type()->is_murky());
        auto module = current_module_->get_module_from_alias(l->get_name());
        if (!module.has_value()) {
            auto err = "alias '" + l->get_name() + "' not recognised for type declaration";
            handler_->report_error(current_filename_, all_errors_[38], err, unmurk_pos);
            return std::make_shared<Type>(TypeSpec::ERROR);
        }

        curr_module_access_ = *module;
        curr_module_alias_ = l->get_name();
        return unmurk_direct(std::dynamic_pointer_cast<MurkyType>(l->get_sub_type()));
        curr_module_access_ = nullptr;
    }
    return murky_t;
}

auto Verifier::unmurk_direct(std::shared_ptr<MurkyType> murky_t) -> std::shared_ptr<Type> {
    auto lex = std::string{};
    lex += murky_t->get_name();

    std::shared_ptr<Module> mod;
    if (curr_module_access_) {
        mod = curr_module_access_;
    }
    else {
        mod = current_module_;
    }

    for (auto& enum_ : mod->get_enums()) {
        if (enum_->get_ident() == lex) {
            enum_->set_used();
            return std::make_shared<EnumType>(enum_);
        }
    }

    for (auto& class_ : mod->get_classes()) {
        if (class_->get_ident() == lex) {
            class_->set_used();
            return std::make_shared<ClassType>(class_);
        }
    }
    auto err = std::string{};
    if (curr_module_access_) {
        err += curr_module_alias_ + "::";
    }
    err += murky_t->get_name();

    handler_->report_error(current_filename_, all_errors_[42], err, unmurk_pos);
    return std::make_shared<Type>(TypeSpec::ERROR);
}