#include "./module.hpp"

auto operator<<(std::ostream& os, Module const& mod) -> std::ostream& {
    os << "Module " << mod.get_filepath() << "\n";

    for (auto const& import : mod.get_imported_filepaths()) {
        os << "  Import: " << import.first << "\n";
    }
    for (auto const& using_ : mod.get_using_filepaths()) {
        os << "  Using: " << using_.first << "\n";
    }

    for (auto const& class_ : mod.get_classes()) {
        class_->print(os);
    }

    for (auto const& global_var : mod.get_global_vars()) {
        global_var->print(os);
    }

    for (auto const& extern_ : mod.get_externs()) {
        extern_->print(os);
    }

    for (auto const& function : mod.get_functions()) {
        function->print(os);
    }
    return os;
}

auto Module::get_constructor_decl(std::shared_ptr<ConstructorCallExpr> constructor_call_expr, bool is_recursive) const
    -> std::optional<std::shared_ptr<ConstructorDecl>> {
    auto it = std::find_if(classes_.begin(), classes_.end(), [&constructor_call_expr](auto const& class_) {
        return class_->get_ident() == constructor_call_expr->get_name();
    });
    if (it != classes_.end()) {
        for (auto& constructor : (*it)->get_constructors()) {
            auto const& call_args = constructor_call_expr->get_args();
            auto const& constructor_args = constructor->get_paras();
            if (call_args.size() != constructor_args.size()) {
                continue;
            }
            bool match = true;
            for (auto i = 0u; i < call_args.size(); ++i) {
                if (!call_args[i]->get_type()->equal_soft(*constructor_args[i]->get_type())) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return constructor;
            }
        }
    }

    if (is_recursive) {
        for (auto& mod : using_modules) {
            auto constructor = mod.second->get_constructor_decl(constructor_call_expr, false);
            if (constructor) {
                return constructor;
            }
        }
    }

    return std::nullopt;
}

auto Module::get_decl(std::shared_ptr<CallExpr> call_expr, bool is_recursive) const
    -> std::optional<std::shared_ptr<Decl>> {
    auto it = std::find_if(functions_.begin(), functions_.end(), [&call_expr](auto const& func) {
        auto dot_pos = func->get_ident().find('.');
        auto prefix = func->get_ident().substr(0, dot_pos);
        if (prefix != call_expr->get_name()) {
            return false;
        }
        auto const& call_args = call_expr->get_args();
        auto const& func_args = func->get_paras();
        if (call_args.size() != func_args.size()) {
            return false;
        }
        for (auto i = 0u; i < call_args.size(); ++i) {
            if (!func_args[i]->get_type()->equal_soft(*call_args[i]->get_type())) {
                return false;
            }
        }
        return true;
    });
    if (it != functions_.end()) {
        return std::optional{*it};
    }

    auto it2 = std::find_if(externs_.begin(), externs_.end(), [&call_expr](auto const& extern_) {
        if (extern_->get_ident() != call_expr->get_name()) {
            return false;
        }
        auto const& call_args = call_expr->get_args();
        auto const& func_args = extern_->get_types();

        if (extern_->is_variatic()) {
            if (call_args.size() < func_args.size() - 1) {
                return false;
            }
        }
        else {
            if (call_args.size() != func_args.size()) {
                return false;
            }
        }

        for (auto i = 0u; i < func_args.size() - 1; ++i) {
            if (!call_args[i]->get_type()->equal_soft(*func_args[i])) {
                return false;
            }
        }
        return true;
    });

    if (it2 != externs_.end()) {
        return std::optional{*it2};
    }

    if (is_recursive) {
        for (auto& mod : using_modules) {
            auto decl = mod.second->get_decl(call_expr, false);
            if (decl) {
                return decl;
            }
        }
    }

    return std::nullopt;
}

auto Module::get_enum(std::string enum_name) const -> std::optional<std::shared_ptr<EnumDecl>> {
    auto it = std::find_if(enums_.begin(), enums_.end(), [&enum_name](auto const& enum_) {
        return enum_->get_ident() == enum_name;
    });
    return (it != enums_.end()) ? std::optional{*it} : std::nullopt;
}