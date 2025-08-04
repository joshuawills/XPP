#include "./type.hpp"

#include <iostream>
#include <map>

auto type_spec_from_lexeme(std::string& lexeme) -> std::optional<TypeSpec> {
    auto const lexeme_to_spec_map = std::map<std::string, TypeSpec>{{"void", TypeSpec::VOID}, {"i64", TypeSpec::I64}};
    auto it = lexeme_to_spec_map.find(lexeme);
    if (it != lexeme_to_spec_map.end()) {
        return it->second;
    }
    return std::nullopt;
}