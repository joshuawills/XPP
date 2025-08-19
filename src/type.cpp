#include "./type.hpp"
#include "./decl.hpp"

#include <iostream>
#include <map>

auto operator<<(std::ostream& os, Type const& t) -> std::ostream& {
    t.print(os);
    return os;
}

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

    auto floats = std::vector<TypeSpec>{TypeSpec::F32, TypeSpec::F64};
    auto is_float = [floats](TypeSpec const& a) -> bool {
        return std::find(floats.begin(), floats.end(), a) != floats.end();
    };

    if (is_float(a) and is_float(b))
        return true;

    return a == b;
}

auto type_spec_from_lexeme(std::string const& lexeme) -> TypeSpec {
    auto const lexeme_to_spec_map = std::map<std::string, TypeSpec>{{"void", TypeSpec::VOID},
                                                                    {"i64", TypeSpec::I64},
                                                                    {"i32", TypeSpec::I32},
                                                                    {"i8", TypeSpec::I8},
                                                                    {"u64", TypeSpec::U64},
                                                                    {"u32", TypeSpec::U32},
                                                                    {"u8", TypeSpec::U8},
                                                                    {"f64", TypeSpec::F64},
                                                                    {"f32", TypeSpec::F32},
                                                                    {"...", TypeSpec::VARIATIC},
                                                                    {"bool", TypeSpec::BOOL}};
    auto it = lexeme_to_spec_map.find(lexeme);
    if (it != lexeme_to_spec_map.end()) {
        return it->second;
    }
    return TypeSpec::MURKY;
}

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream& {
    switch (ts) {
    case TypeSpec::VOID: os << "void"; break;
    case TypeSpec::I64: os << "i64"; break;
    case TypeSpec::I32: os << "i32"; break;
    case TypeSpec::U64: os << "u64"; break;
    case TypeSpec::U32: os << "u32"; break;
    case TypeSpec::F64: os << "f64"; break;
    case TypeSpec::F32: os << "f32"; break;
    case TypeSpec::BOOL: os << "bool"; break;
    case TypeSpec::POINTER: os << "*"; break;
    case TypeSpec::I8: os << "i8"; break;
    case TypeSpec::U8: os << "u8"; break;
    case TypeSpec::VARIATIC: os << "..."; break;
    case TypeSpec::UNKNOWN: os << "unknown"; break;
    case TypeSpec::ARRAY: os << "array"; break;
    case TypeSpec::ENUM: os << "enum"; break;
    case TypeSpec::IMPORT: os << "import"; break;
    default: os << "invalid typespec"; break;
    }
    return os;
}

EnumType::EnumType()
: Type(TypeSpec::ENUM) {}

EnumType::EnumType(std::shared_ptr<EnumDecl> ref)
: Type(TypeSpec::ENUM)
, ref_(ref) {}

auto EnumType::set_ref(std::shared_ptr<EnumDecl> ref) -> void {
    ref_ = ref;
}

auto EnumType::get_ref() const -> std::shared_ptr<EnumDecl> {
    return ref_;
}

auto EnumType::print(std::ostream& os) const -> void {
    ref_->print(os);
}

auto EnumType::equals(const Type& other) const -> bool {
    if (t_ != other.get_type_spec())
        return false;

    auto* other_ptr = dynamic_cast<const EnumType*>(&other);
    if (!other_ptr)
        return false;

    return ref_->get_ident() == other_ptr->get_ref()->get_ident()
           and ref_->get_fields() == other_ptr->get_ref()->get_fields();
}

auto EnumType::equal_soft(const Type& other) const -> bool {
    return equals(other);
}

auto ClassType::print(std::ostream& os) const -> void {
    os << "classtype_" << ref_->get_ident();
}

auto ClassType::equals(const Type& other) const -> bool {
    if (t_ != other.get_type_spec())
        return false;

    auto* other_ptr = dynamic_cast<const ClassType*>(&other);
    if (!other_ptr)
        return false;

    return other_ptr->get_ref() == get_ref();
}

auto ClassType::equal_soft(const Type& other) const -> bool {
    return equals(other);
}

auto ClassType::get_ref() const -> std::shared_ptr<ClassDecl> {
    return ref_;
}

auto ClassType::set_ref(std::shared_ptr<ClassDecl> ref) -> void {
    ref_ = ref;
}

auto MurkyType::print(std::ostream& os) const -> void {
    os << name_;
}
auto MurkyType::equals(const Type& other) const -> bool {
    (void)other;
    return false;
}
auto MurkyType::equal_soft(const Type& other) const -> bool {
    (void)other;
    return false;
}