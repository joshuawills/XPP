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

    auto get_filepath() const -> std::string const& {
        return filepath_;
    }

    auto get_functions() const -> std::vector<std::shared_ptr<Function>> {
        return functions_;
    }

    auto function_with_name_exists(std::string const& name) const -> bool {
        return std::any_of(functions_.begin(), functions_.end(), [&name](auto const& func) {
            return func->get_ident() == name;
        });
    }

    auto get_function(std::shared_ptr<CallExpr> call_expr) const -> std::optional<std::shared_ptr<Function>>;

 private:
    std::string filepath_;
    std::vector<std::shared_ptr<Function>> functions_ = {};
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
    }

    auto module_exists_from_filename(std::string const& filename) const -> bool {
        for (const auto& module : modules_) {
            if (module->get_filepath() == filename) {
                return true;
            }
        }
        return false;
    }

    auto get_main_module() const -> std::shared_ptr<Module> {
        return main_module_;
    }

 private:
    std::vector<std::shared_ptr<Module>> modules_ = {};
    std::shared_ptr<Module> main_module_ = nullptr;
};

#endif // MODULE_HPP