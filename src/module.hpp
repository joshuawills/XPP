#ifndef MODULE_HPP
#define MODULE_HPP

#include <algorithm>

#include "./ast.hpp"
#include "./decl.hpp"

class Module {
 public:
    Module(std::string filepath)
    : filepath_(filepath) {}

    auto add_function(std::shared_ptr<Function> func) -> void {
        functions_.push_back(func);
    }

    auto add_extern(std::shared_ptr<Extern> extern_) -> void {
        externs_.push_back(extern_);
    }

    auto add_global_var(std::shared_ptr<GlobalVarDecl> global_var) -> void {
        global_vars_.push_back(global_var);
    }

    auto get_filepath() const -> std::string const& {
        return filepath_;
    }

    auto get_functions() const -> std::vector<std::shared_ptr<Function>> {
        return functions_;
    }

    auto get_externs() const -> std::vector<std::shared_ptr<Extern>> {
        return externs_;
    }

    auto get_global_vars() const -> std::vector<std::shared_ptr<GlobalVarDecl>> {
        return global_vars_;
    }

    auto function_with_name_exists(std::string const& name) const -> bool {
        auto const user_func = std::any_of(functions_.begin(), functions_.end(), [&name](auto const& func) {
            return func->get_ident() == name;
        });
        if (user_func)
            return true;
        return std::any_of(externs_.begin(), externs_.end(), [&name](auto const& extern_) {
            return extern_->get_ident() == name;
        });
    }

    auto get_decl(std::shared_ptr<CallExpr> call_expr) const -> std::optional<std::shared_ptr<Decl>>;

 private:
    std::string filepath_;
    std::vector<std::shared_ptr<Function>> functions_ = {};
    std::vector<std::shared_ptr<Extern>> externs_ = {};
    std::vector<std::shared_ptr<GlobalVarDecl>> global_vars_ = {};
};

auto operator<<(std::ostream& os, Module const& mod) -> std::ostream&;

class AllModules {
 public:
    AllModules() = default;

    auto add_module(std::shared_ptr<Module> module) -> void {
        modules_.push_back(module);
    }

    auto add_main_module(std::shared_ptr<Module> module) -> void {
        main_module_ = module;
        add_module(module);
    }

    auto module_exists_from_filename(std::string const& filename) const -> bool {
        for (const auto& module : modules_) {
            if (module->get_filepath() == filename) {
                return true;
            }
        }
        return false;
    }

    auto get_modules() -> std::vector<std::shared_ptr<Module>> {
        return modules_;
    }

    auto get_main_module() const -> std::shared_ptr<Module> {
        return main_module_;
    }

 private:
    std::vector<std::shared_ptr<Module>> modules_ = {};
    std::shared_ptr<Module> main_module_ = nullptr;
};

#endif // MODULE_HPP