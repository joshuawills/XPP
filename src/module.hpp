#ifndef MODULE_HPP
#define MODULE_HPP

#include "./ast.hpp"
#include "./decl.hpp"

class Module {
    public:
        Module(std::string filepath):
            filepath_(filepath) {}

        auto add_function(std::shared_ptr<Function> func) -> void {
            functions_.push_back(func);
        }
    private:
        std::string filepath_;
        std::vector<std::shared_ptr<Function>> functions_ = {};
};

#endif // MODULE_HPP