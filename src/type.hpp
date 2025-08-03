#ifndef TYPE_HPP
#define TYPE_HPP

#include "./ast.hpp"

enum TypeSpec {
    VOID,
    I64,
    UNKNOWN,
};

struct Type {
    TypeSpec t;
    std::optional<std::string> lexeme;
};

auto type_spec_from_lexeme(std::string& lexeme) -> std::optional<TypeSpec>;

#endif // TYPE_HPP