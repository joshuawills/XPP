#ifndef TYPE_HPP
#define TYPE_HPP

#include "./ast.hpp"

enum TypeSpec { VOID, I64, I32, BOOL, UNKNOWN, ERROR, POINTER, I8, VARIATIC, U64, U32, U8, F32, F64 };

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream&;

struct Type {
    TypeSpec t;
    std::optional<std::string> lexeme = std::nullopt;
    std::shared_ptr<Type> sub_type = nullptr;

    Type(TypeSpec t, std::optional<std::string> lex = std::nullopt, std::shared_ptr<Type> sub = nullptr)
    : t(t)
    , lexeme(std::move(lex))
    , sub_type(std::move(sub)) {}

    auto equal_soft(const Type& other) const -> bool;

    auto operator==(const Type& other) const -> bool;

    auto operator!=(const Type& other) const -> bool {
        return !(*this == other);
    }

    auto get_type_spec() const -> TypeSpec {
        return t;
    }

    auto is_numeric() const noexcept -> bool {
        return is_signed_int() or is_unsigned_int() or is_decimal();
    }

    auto is_int() const noexcept -> bool {
        return is_signed_int() or is_unsigned_int();
    }

    auto is_signed_int() const noexcept -> bool {
        return t == TypeSpec::I64 or t == TypeSpec::I32 or t == TypeSpec::I8;
    }

    auto is_unsigned_int() const noexcept -> bool {
        return t == TypeSpec::U64 or t == TypeSpec::U32 or t == TypeSpec::U8;
    }

    auto is_decimal() const noexcept -> bool {
        return t == TypeSpec::F32 or t == TypeSpec::F64;
    }

    auto is_pointer() const noexcept -> bool {
        return t == TypeSpec::POINTER;
    }

    auto is_bool() const noexcept -> bool {
        return t == TypeSpec::BOOL;
    }
};

auto operator<<(std::ostream& os, Type const& t) -> std::ostream&;

auto type_spec_from_lexeme(std::string const& lexeme) -> std::optional<TypeSpec>;

#endif // TYPE_HPP