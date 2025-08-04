#include "./type.hpp"

#include <iostream>
#include <map>

auto type_spec_from_lexeme(std::string& lexeme) -> std::optional<TypeSpec> {
    auto const lexeme_to_spec_map =
        std::map<std::string, TypeSpec>{{"void", TypeSpec::VOID}, {"i64", TypeSpec::I64}, {"bool", TypeSpec::BOOL}};
    auto it = lexeme_to_spec_map.find(lexeme);
    if (it != lexeme_to_spec_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

auto type_spec_to_string(TypeSpec t) -> std::string {
    switch (t) {
    case TypeSpec::VOID: return "void";
    case TypeSpec::I64: return "i64";
    case TypeSpec::BOOL: return "bool";
    case TypeSpec::UNKNOWN: return "unknown";
    default: return "invalid";
    }
}