#ifndef TYPE_HPP
#define TYPE_HPP

#include "./ast.hpp"

enum TypeSpec {
    VOID,
    I64,
    BOOL,
    UNKNOWN,
    ERROR,
};

struct Type {
    TypeSpec t;
    std::optional<std::string> lexeme;

    auto operator==(const Type& other) const -> bool {
        return t == other.t && lexeme == other.lexeme;
    }

    auto operator!=(const Type& other) const -> bool {
        return !(*this == other);
    }

    auto get_type_spec() const -> TypeSpec {
        return t;
    }
};

auto type_spec_from_lexeme(std::string& lexeme) -> std::optional<TypeSpec>;
auto type_spec_to_string(TypeSpec t) -> std::string;

#endif // TYPE_HPP