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

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream&;

struct Type {
    TypeSpec t;
    std::optional<std::string> lexeme;

    auto operator==(const Type& other) const -> bool {
        return t == other.t and lexeme == other.lexeme;
    }

    auto operator!=(const Type& other) const -> bool {
        return !(*this == other);
    }

    auto get_type_spec() const -> TypeSpec {
        return t;
    }
};

auto operator<<(std::ostream& os, Type const& t) -> std::ostream&;

auto type_spec_from_lexeme(std::string const& lexeme) -> std::optional<TypeSpec>;

#endif // TYPE_HPP