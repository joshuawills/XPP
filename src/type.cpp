#include "./type.hpp"

#include <iostream>
#include <map>

auto operator<<(std::ostream& os, Type const& t) -> std::ostream& {
    os << t.t;
    if (t.lexeme.has_value()) {
        os << ", " << *t.lexeme;
    }
    return os;
}

auto type_spec_from_lexeme(std::string const& lexeme) -> std::optional<TypeSpec> {
    auto const lexeme_to_spec_map =
        std::map<std::string, TypeSpec>{{"void", TypeSpec::VOID}, {"i64", TypeSpec::I64}, {"bool", TypeSpec::BOOL}};
    auto it = lexeme_to_spec_map.find(lexeme);
    if (it != lexeme_to_spec_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream& {
    switch (ts) {
    case TypeSpec::VOID: os << "void"; break;
    case TypeSpec::I64: os << "i64"; break;
    case TypeSpec::BOOL: os << "bool"; break;
    case TypeSpec::UNKNOWN: os << "unknown"; break;
    default: os << "invalid typespec"; break;
    }
    return os;
}