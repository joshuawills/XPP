#ifndef TYPE_HPP
#define TYPE_HPP

#include <iostream>
#include <sstream>

#include "./ast.hpp"

enum TypeSpec {
    VOID,
    I64,
    I32,
    BOOL,
    UNKNOWN,
    ERROR,
    POINTER,
    I8,
    VARIATIC,
    U64,
    U32,
    U8,
    F32,
    F64,
    ARRAY,
    ENUM,
    MURKY,
    CLASS,
    IMPORT
};

auto operator<<(std::ostream& os, TypeSpec const& ts) -> std::ostream&;

auto soft_typespec_equals(TypeSpec const& a, TypeSpec const& b) -> bool;

class Type {
 public:
    Type(TypeSpec t)
    : t_(t) {}

    Type()
    : t_(TypeSpec::UNKNOWN) {}

    virtual ~Type() = default;

    virtual auto equals(const Type& other) const -> bool {
        return t_ == other.t_;
    }

    virtual auto equal_soft(const Type& other) const -> bool {
        return soft_typespec_equals(t_, other.get_type_spec());
    }

    auto operator==(const Type& other) -> bool {
        return equals(other);
    }

    auto operator!=(const Type& other) -> bool {
        return !equals(other);
    }

    auto to_string() const -> std::string {
        auto s = std::stringstream{};
        print(s);
        return s.str();
    }

    virtual auto print(std::ostream& os) const -> void {
        os << t_;
    }

    auto get_type_spec() const -> TypeSpec {
        return t_;
    }

    auto is_primitive() const noexcept -> bool {
        auto primitive_types = std::vector<TypeSpec>{TypeSpec::I64,
                                                     TypeSpec::I32,
                                                     TypeSpec::I8,
                                                     TypeSpec::U64,
                                                     TypeSpec::U32,
                                                     TypeSpec::U8,
                                                     TypeSpec::F32,
                                                     TypeSpec::F64,
                                                     TypeSpec::BOOL,
                                                     TypeSpec::POINTER,
                                                     TypeSpec::ARRAY};
        return std::find(primitive_types.begin(), primitive_types.end(), t_) != primitive_types.end();
    }

    auto is_variatic() const noexcept -> bool {
        return t_ == TypeSpec::VARIATIC;
    }

    auto is_unknown() const noexcept -> bool {
        return t_ == TypeSpec::UNKNOWN;
    }

    auto is_void() const noexcept -> bool {
        return t_ == TypeSpec::VOID;
    }

    auto is_error() const noexcept -> bool {
        return t_ == TypeSpec::ERROR;
    }

    auto is_numeric() const noexcept -> bool {
        return is_signed_int() or is_unsigned_int() or is_decimal();
    }

    auto is_int() const noexcept -> bool {
        return is_signed_int() or is_unsigned_int();
    }

    auto is_signed_int() const noexcept -> bool {
        return t_ == TypeSpec::I64 or t_ == TypeSpec::I32 or t_ == TypeSpec::I8;
    }

    auto is_i64() const noexcept -> bool {
        return t_ == TypeSpec::I64;
    }

    auto is_unsigned_int() const noexcept -> bool {
        return t_ == TypeSpec::U64 or t_ == TypeSpec::U32 or t_ == TypeSpec::U8;
    }

    auto is_decimal() const noexcept -> bool {
        return t_ == TypeSpec::F32 or t_ == TypeSpec::F64;
    }

    auto is_pointer() const noexcept -> bool {
        return t_ == TypeSpec::POINTER;
    }

    auto is_bool() const noexcept -> bool {
        return t_ == TypeSpec::BOOL;
    }

    auto is_array() const noexcept -> bool {
        return t_ == TypeSpec::ARRAY;
    }

    auto is_enum() const noexcept -> bool {
        return t_ == TypeSpec::ENUM;
    }

    auto is_murky() const noexcept -> bool {
        return t_ == TypeSpec::MURKY;
    }

    auto is_class() const noexcept -> bool {
        return t_ == TypeSpec::CLASS;
    }

    auto is_import() const noexcept -> bool {
        return t_ == TypeSpec::IMPORT;
    }

 protected:
    TypeSpec t_;
};

class ArrayType : public Type {
 public:
    ArrayType(std::shared_ptr<Type> sub_type)
    : Type(TypeSpec::ARRAY)
    , sub_type_(sub_type) {}

    ArrayType(std::shared_ptr<Type> sub_type, size_t length)
    : Type(TypeSpec::ARRAY)
    , sub_type_(sub_type)
    , length_(std::optional(length)) {}

    auto get_sub_type() const -> std::shared_ptr<Type> {
        return sub_type_;
    }

    auto get_length() const -> std::optional<size_t> {
        return length_;
    }

    auto print(std::ostream& os) const -> void override {
        os << t_;
        sub_type_->print(os);
    }

    auto equals(const Type& other) const -> bool override {
        if (t_ != other.get_type_spec())
            return false;

        auto* other_ptr = dynamic_cast<const ArrayType*>(&other);
        if (!other_ptr)
            return false;

        if (length_.has_value() and other_ptr->get_length().has_value()) {
            return *sub_type_ == *other_ptr->sub_type_ and *length_ == *other_ptr->get_length();
        }
        else {
            return *sub_type_ == *other_ptr->sub_type_;
        }
    }

    auto equal_soft(const Type& other) const -> bool override {
        auto* other_ptr = dynamic_cast<const ArrayType*>(&other);
        if (!other_ptr) {
            return false;
        }
        return soft_typespec_equals(t_, other.get_type_spec()) and sub_type_->equal_soft(*other_ptr->get_sub_type());
    }

 private:
    std::shared_ptr<Type> sub_type_;
    std::optional<size_t> length_ = std::nullopt;
};

class PointerType : public Type {
 public:
    PointerType(std::shared_ptr<Type> sub_type)
    : Type(TypeSpec::POINTER)
    , sub_type_(sub_type) {}

    auto get_sub_type() const -> std::shared_ptr<Type> {
        return sub_type_;
    }

    auto print(std::ostream& os) const -> void override {
        os << t_;
        sub_type_->print(os);
    }

    auto equals(const Type& other) const -> bool override {
        if (t_ != other.get_type_spec())
            return false;

        auto* other_ptr = dynamic_cast<const PointerType*>(&other);
        if (!other_ptr)
            return false;

        if (sub_type_->is_void() or other_ptr->sub_type_->is_void()) {
            return true;
        }
        return *sub_type_ == *other_ptr->sub_type_;
    }

    auto equal_soft(const Type& other) const -> bool override {
        auto* array_ptr = dynamic_cast<const ArrayType*>(&other);
        if (array_ptr) {
            return *sub_type_ == *array_ptr->get_sub_type();
        }

        if (t_ != other.get_type_spec())
            return false;

        auto* other_ptr = dynamic_cast<const PointerType*>(&other);
        if (!other_ptr)
            return false;

        if (sub_type_->is_void() or other_ptr->sub_type_->is_void()) {
            return true;
        }
        return *sub_type_ == *other_ptr->sub_type_;
    }

 private:
    std::shared_ptr<Type> sub_type_;
};

class EnumType : public Type {
 public:
    EnumType();
    EnumType(std::shared_ptr<EnumDecl> ref);

    auto set_ref(std::shared_ptr<EnumDecl> ref) -> void;
    auto get_ref() const -> std::shared_ptr<EnumDecl>;

    auto print(std::ostream& os) const -> void override;
    auto equals(const Type& other) const -> bool override;
    auto equal_soft(const Type& other) const -> bool override;

 private:
    std::shared_ptr<EnumDecl> ref_ = nullptr;
};

class ClassType : public Type {
 public:
    ClassType();
    ClassType(std::shared_ptr<ClassDecl> ref)
    : Type(TypeSpec::CLASS)
    , ref_(ref) {}

    auto get_ref() const -> std::shared_ptr<ClassDecl>;
    auto set_ref(std::shared_ptr<ClassDecl> ref) -> void;

    auto print(std::ostream& os) const -> void override;
    auto equals(const Type& other) const -> bool override;
    auto equal_soft(const Type& other) const -> bool override;

 private:
    std::shared_ptr<ClassDecl> ref_ = nullptr;
};

class ImportType : public Type {
 public:
    ImportType(std::string import_path, std::shared_ptr<Type> sub_type)
    : Type(TypeSpec::IMPORT)
    , import_path_(import_path)
    , sub_type_(sub_type) {}

    auto get_sub_type() const -> std::shared_ptr<Type> {
        return sub_type_;
    }

    auto print(std::ostream& os) const -> void override {
        os << import_path_ << "::";
        sub_type_->print(os);
    }

    auto equals(const Type& other) const -> bool override {
        auto* import_ptr = dynamic_cast<const ImportType*>(&other);
        if (import_ptr) {
            return *sub_type_ == *import_ptr->get_sub_type();
        }
        else {
            return *sub_type_ == other;
        }
    }

    auto equal_soft(const Type& other) const -> bool override {
        auto* import_ptr = dynamic_cast<const ImportType*>(&other);
        if (import_ptr) {
            return sub_type_->equal_soft(*import_ptr->get_sub_type());
        }
        else {
            return sub_type_->equal_soft(other);
        }
    }

    auto get_name() const -> std::string {
        return import_path_;
    }

 private:
    std::string import_path_;
    std::shared_ptr<Type> sub_type_ = nullptr;
};

class MurkyType : public Type {
 public:
    MurkyType(std::string const name)
    : Type(TypeSpec::MURKY)
    , name_(name) {}

    auto get_name() const -> std::string {
        return name_;
    }

    auto print(std::ostream& os) const -> void override;
    auto equals(const Type& other) const -> bool override;
    auto equal_soft(const Type& other) const -> bool override;

 private:
    std::string const name_;
};

auto operator<<(std::ostream& os, Type const& t) -> std::ostream&;

auto type_spec_from_lexeme(std::string const& lexeme) -> TypeSpec;

#endif // TYPE_HPP