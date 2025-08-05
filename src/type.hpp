#ifndef TYPE_HPP
#define TYPE_HPP

#include "./ast.hpp"

enum TypeSpec { VOID, I64, I32, BOOL, UNKNOWN, ERROR, POINTER, I8, VARIATIC };

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream&;

struct Type {
    TypeSpec t;
    std::optional<std::string> lexeme = std::nullopt;
    std::shared_ptr<Type> sub_type = nullptr;

    Type(TypeSpec t, std::optional<std::string> lex = std::nullopt, std::shared_ptr<Type> sub = nullptr)
    : t(t)
    , lexeme(std::move(lex))
    , sub_type(std::move(sub)) {}

    auto operator==(const Type& other) const -> bool;

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