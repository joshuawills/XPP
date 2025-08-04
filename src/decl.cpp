#include "decl.hpp"

#include <iostream>

auto Function::operator==(const Function& other) const -> bool {
    if (this == &other) {
        return true;
    }
    if (ident_ != other.ident_) {
        return false;
    }
    if (t_ != other.t_) {
        return false;
    }
    if (paras_.size() != other.paras_.size()) {
        return false;
    }
    for (size_t i = 0; i < paras_.size(); ++i) {
        if (*paras_[i] != *other.paras_[i]) {
            return false;
        }
    }
    return true;
}