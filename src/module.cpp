#include "./module.hpp"

auto operator<<(std::ostream& os, Module const& mod) -> std::ostream& {
    os << "Module " << mod.get_filepath() << "\n";
    for (auto const& function : mod.get_functions()) {
        function->print(os);
    }
    return os;
}

auto Module::get_function(std::shared_ptr<CallExpr> call_expr) const -> std::optional<std::shared_ptr<Function>> {
    auto it = std::find_if(functions_.begin(), functions_.end(), [&call_expr](auto const& func) {
        if (func->get_ident() != call_expr->get_name()) {
            return false;
        }
        auto const& call_args = call_expr->get_args();
        auto const& func_args = func->get_paras();
        if (call_args.size() != func_args.size()) {
            return false;
        }
        for (auto i = 0u; i < call_args.size(); ++i) {
            if (call_args[i]->get_type() != func_args[i]->get_type()) {
                return false;
            }
        }
        return true;
    });
    return (it == functions_.end()) ? std::nullopt : std::optional{*it};
}