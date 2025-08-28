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

    auto add_enums(std::shared_ptr<EnumDecl> enum_) -> void {
        enums_.push_back(enum_);
    }

    auto add_class(std::shared_ptr<ClassDecl> class_) -> void {
        classes_.push_back(class_);
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

    auto get_global_var(std::string const& name) -> std::optional<std::shared_ptr<GlobalVarDecl>> {
        auto it = std::find_if(global_vars_.begin(), global_vars_.end(), [&name](auto const& global_var) {
            return global_var->get_ident() == name;
        });
        if (it != global_vars_.end()) {
            return std::optional{*it};
        }
        return std::nullopt;
    }

    auto get_enums() const -> std::vector<std::shared_ptr<EnumDecl>> {
        return enums_;
    }

    auto get_classes() const -> std::vector<std::shared_ptr<ClassDecl>> {
        return classes_;
    }

    auto get_imported_filepaths() const -> std::vector<std::pair<std::string, bool>> {
        return imported_files;
    }

    auto get_using_filepaths() const -> std::vector<std::pair<std::string, bool>> {
        return using_files;
    }

    auto add_imported_filepath(std::string const& alias, std::string const& filepath, bool is_libc = false) -> void {
        imported_files.push_back({filepath, is_libc});
        alias_import_to_path[alias] = filepath;
    }

    auto add_using_filepath(std::string const& filepath, bool is_libc = false) -> void {
        using_files.push_back({filepath, is_libc});
    }

    auto class_with_name_exists(std::string const& name) const -> bool {
        return std::any_of(classes_.begin(), classes_.end(), [&name](auto const& class_) {
            return class_->get_ident() == name;
        });
    }

    // NOTE: considers using modules second
    auto function_with_name_exists(std::string const& name) const -> bool {
        auto const user_func = std::any_of(functions_.begin(), functions_.end(), [&name](auto const& func) {
            return func->get_ident() == name;
        });
        if (user_func)
            return true;
        auto const extern_func = std::any_of(externs_.begin(), externs_.end(), [&name](auto const& extern_) {
            return extern_->get_ident() == name;
        });
        if (extern_func)
            return true;
        for (auto& mod : using_modules) {
            if (mod.second->function_with_name_exists(name)) {
                return true;
            }
        }
        return false;
    }

    auto get_module_from_alias(std::string const& alias) const -> std::optional<std::shared_ptr<Module>> {
        auto it = alias_import_to_path.find(alias);
        if (it != alias_import_to_path.end()) {
            return get_module_from_filepath(it->second);
        }
        return std::nullopt;
    }

    auto get_module_from_filepath(std::string const& filepath) const -> std::optional<std::shared_ptr<Module>> {
        for (const auto& module : imported_modules) {
            if (module.second->get_filepath() == filepath) {
                return module.second;
            }
        }
        return std::nullopt;
    }

    auto add_imported_module(std::string const& name, std::shared_ptr<Module> module) -> void {
        imported_modules[name] = module;
    }

    auto add_using_module(std::string const& name, std::shared_ptr<Module> module) -> void {
        using_modules[name] = module;
    }

    auto get_using_modules() -> std::vector<std::shared_ptr<Module>> {
        std::vector<std::shared_ptr<Module>> modules;
        for (auto const& using_module : using_modules) {
            modules.push_back(using_module.second);
        }
        return modules;
    }

    auto get_decl(std::shared_ptr<CallExpr> call_expr, bool is_recursive) const -> std::optional<std::shared_ptr<Decl>>;
    auto get_constructor_decl(std::shared_ptr<ConstructorCallExpr> constructor_call_expr, bool is_recursive) const
        -> std::optional<std::shared_ptr<ConstructorDecl>>;
    auto get_enum(std::string enum_name) const -> std::optional<std::shared_ptr<EnumDecl>>;

    auto set_is_lib(bool is_lib) -> void {
        is_lib_ = is_lib;
    }

    auto is_lib() const -> bool {
        return is_lib_;
    }

 private:
    bool is_lib_ = false;
    std::string filepath_;
    std::vector<std::shared_ptr<Function>> functions_ = {};
    std::vector<std::shared_ptr<Extern>> externs_ = {};
    std::vector<std::shared_ptr<GlobalVarDecl>> global_vars_ = {};
    std::vector<std::shared_ptr<EnumDecl>> enums_ = {};
    std::vector<std::shared_ptr<ClassDecl>> classes_ = {};

    std::map<std::string, std::string> alias_import_to_path = {};
    std::vector<std::pair<std::string, bool>> imported_files = {};
    std::vector<std::pair<std::string, bool>> using_files = {};
    std::map<std::string, std::shared_ptr<Module>> imported_modules = {};
    std::map<std::string, std::shared_ptr<Module>> using_modules = {};
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

    auto get_module_from_filepath(std::string const& filepath) const -> std::shared_ptr<Module> {
        for (const auto& module : modules_) {
            if (module->get_filepath() == filepath) {
                return module;
            }
        }
        return nullptr;
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