#include "./type.hpp"

#include <iostream>
#include <map>

auto soft_typespec_equals(TypeSpec const& a, TypeSpec const& b) -> bool {
    auto signed_int = std::vector<TypeSpec>{TypeSpec::I8, TypeSpec::I32, TypeSpec::I64};
    auto is_signed_int = [signed_int](TypeSpec const& a) -> bool {
        return std::find(signed_int.begin(), signed_int.end(), a) != signed_int.end();
    };

    if (is_signed_int(a) and is_signed_int(b))
        return true;

    auto unsigned_int = std::vector<TypeSpec>{TypeSpec::U8, TypeSpec::U32, TypeSpec::U64};
    auto is_unsigned_int = [unsigned_int](TypeSpec const& a) -> bool {
        return std::find(unsigned_int.begin(), unsigned_int.end(), a) != unsigned_int.end();
    };

    if (is_unsigned_int(a) and is_unsigned_int(b))
        return true;

    return a == b;
}

auto Type::equal_soft(const Type& other) const -> bool {
    auto res = true;
    if (sub_type) {
        if (!other.sub_type) {
            return false;
        }
        res &= *sub_type == *(other.sub_type);
    }
    res &= soft_typespec_equals(t, other.t);
    res &= lexeme == other.lexeme;
    return res;
}

auto Type::operator==(const Type& other) const -> bool {
    auto res = true;
    if (sub_type) {
        if (!other.sub_type) {
            return false;
        }
        res &= *sub_type == *(other.sub_type);
    }
    res &= t == other.t;
    res &= lexeme == other.lexeme;
    return res;
}

auto operator<<(std::ostream& os, Type const& t) -> std::ostream& {
    if (t.sub_type != nullptr) {
        os << *t.sub_type;
    }
    os << t.t;
    if (t.lexeme.has_value()) {
        os << ", " << *t.lexeme;
    }
    return os;
}

auto type_spec_from_lexeme(std::string const& lexeme) -> std::optional<TypeSpec> {
    auto const lexeme_to_spec_map = std::map<std::string, TypeSpec>{{"void", TypeSpec::VOID},
                                                                    {"i64", TypeSpec::I64},
                                                                    {"i32", TypeSpec::I32},
                                                                    {"i8", TypeSpec::I8},
                                                                    {"u64", TypeSpec::U64},
                                                                    {"u32", TypeSpec::U32},
                                                                    {"u8", TypeSpec::U8},
                                                                    {"...", TypeSpec::VARIATIC},
                                                                    {"bool", TypeSpec::BOOL}};
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
    case TypeSpec::I32: os << "i32"; break;
    case TypeSpec::U64: os << "u64"; break;
    case TypeSpec::U32: os << "u32"; break;
    case TypeSpec::BOOL: os << "bool"; break;
    case TypeSpec::POINTER: os << "*"; break;
    case TypeSpec::I8: os << "i8"; break;
    case TypeSpec::U8: os << "u8"; break;
    case TypeSpec::VARIATIC: os << "..."; break;
    case TypeSpec::UNKNOWN: os << "unknown"; break;
    default: os << "invalid typespec"; break;
    }
    return os;
}