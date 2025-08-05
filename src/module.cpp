#include "./module.hpp"

auto operator<<(std::ostream& os, Module const& mod) -> std::ostream& {
    os << "Module " << mod.get_filepath() << "\n";
    for (auto const& function : mod.get_functions()) {
        function->print(os);
    }
    return os;
}