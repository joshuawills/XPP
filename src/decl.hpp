#ifndef DECL_HPP
#define DECL_HPP

#include "./ast.hpp"
#include "./expr.hpp"
#include "./stmt.hpp"
#include "./type.hpp"
#include "./visitor.hpp"

#include <sstream>

class Decl : public AST {
 public:
    Decl(Position pos, std::string ident, std::shared_ptr<Type> t)
    : AST(pos)
    , ident_(std::move(ident))
    , t_(t) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;
    auto print(std::ostream& os) const -> void override = 0;

    auto set_type(std::shared_ptr<Type> t) -> void {
        t_ = t;
    }

    auto is_used() const -> bool {
        return is_used_;
    }
    auto set_used() -> void {
        is_used_ = true;
    }

    auto is_reassigned() const -> bool {
        return is_reassigned_;
    }
    auto set_reassigned() -> void {
        is_reassigned_ = true;
    }

    auto is_mut() const -> bool {
        return is_mut_;
    }
    auto set_mut() -> void {
        is_mut_ = true;
    }

    auto is_pub() const -> bool {
        return is_pub_;
    }
    auto set_pub() -> void {
        is_pub_ = true;
    }

    auto get_ident() const -> std::string const& {
        return ident_;
    }
    auto get_type() const -> std::shared_ptr<Type> {
        return t_;
    }

    auto set_statement_num(size_t num) -> void {
        statement_num_ = num;
    }

    auto set_depth_num(size_t num) -> void {
        depth_num_ = num;
    }

    auto get_append() -> std::string {
        return "." + std::to_string(statement_num_) + "_" + std::to_string(depth_num_);
    }

 protected:
    bool is_used_ = false, is_reassigned_ = false, is_mut_ = false, is_pub_ = false;
    std::string ident_;
    std::shared_ptr<Type> t_;
    size_t statement_num_ = 0, depth_num_ = 0;
};

class ParaDecl
: public Decl
, public std::enable_shared_from_this<ParaDecl> {
 public:
    ParaDecl(Position pos, std::string ident, std::shared_ptr<Type> t)
    : Decl(pos, ident, t) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_para_decl(shared_from_this());
    }

    auto operator==(const ParaDecl& other) const -> bool {
        return *t_ == *other.t_;
    }

    auto operator!=(const ParaDecl& other) const -> bool {
        return !(*this == other);
    }

    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;
};

class LocalVarDecl
: public Decl
, public std::enable_shared_from_this<LocalVarDecl> {
 public:
    LocalVarDecl(Position pos, std::string ident, std::shared_ptr<Type> t, std::shared_ptr<Expr> e)
    : Decl(pos, ident, t)
    , expr_(e) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_local_var_decl(shared_from_this());
    }

    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }

    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

 private:
    std::shared_ptr<Expr> expr_;
};

class GlobalVarDecl
: public Decl
, public std::enable_shared_from_this<GlobalVarDecl> {
 public:
    GlobalVarDecl(Position pos, std::string const ident, std::shared_ptr<Type> t, std::shared_ptr<Expr> expr)
    : Decl(pos, ident, t)
    , expr_(expr) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_global_var_decl(shared_from_this());
    }

    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto operator==(GlobalVarDecl const& other) -> bool {
        return get_ident() == other.get_ident();
    }

    auto set_expr(std::shared_ptr<Expr> expr) -> void {
        expr_ = expr;
    }

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

    auto handle_global_arr(std::shared_ptr<Emitter> emitter) -> llvm::Value*;

 private:
    std::shared_ptr<Expr> expr_;
};

class Function
: public Decl
, public std::enable_shared_from_this<Function> {
 public:
    Function(Position const pos,
             std::string const ident,
             std::vector<std::shared_ptr<ParaDecl>> paras,
             std::shared_ptr<Type> t,
             std::shared_ptr<CompoundStmt> const stmts)
    : Decl(pos, ident, t)
    , paras_(paras)
    , stmts_(stmts) {}

    auto get_paras() const -> std::vector<std::shared_ptr<ParaDecl>> const& {
        return paras_;
    }
    auto get_compound_stmt() const -> std::shared_ptr<CompoundStmt> const& {
        return stmts_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_function(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_type_output() -> std::string {
        if (type_output.size() != 0) {
            return type_output;
        }

        auto buffer = std::stringstream{};
        for (auto& para : paras_) {
            buffer << *para->get_type();
        }
        buffer << '.';
        type_output = buffer.str();
        return type_output;
    }

    auto operator==(const Function& other) const -> bool;

 private:
    std::vector<std::shared_ptr<ParaDecl>> const paras_;
    std::shared_ptr<CompoundStmt> const stmts_;
    std::string type_output = "";
};

class MethodDecl
: public Decl
, public std::enable_shared_from_this<MethodDecl> {
 public:
    MethodDecl(Position const pos,
               std::string const ident,
               std::vector<std::shared_ptr<ParaDecl>> paras,
               std::shared_ptr<Type> t,
               std::shared_ptr<CompoundStmt> stmts)
    : Decl(pos, ident, t)
    , paras_(paras)
    , stmts_(stmts) {}
    auto get_paras() const -> std::vector<std::shared_ptr<ParaDecl>> const& {
        return paras_;
    }
    auto get_compound_stmt() const -> std::shared_ptr<CompoundStmt> const& {
        return stmts_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_method_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_type_output() -> std::string {
        if (type_output.size() != 0) {
            return type_output;
        }

        auto buffer = std::stringstream{};
        for (auto& para : paras_) {
            buffer << *para->get_type();
        }
        buffer << '.';
        type_output = buffer.str();
        return type_output;
    }

    auto operator==(const MethodDecl& other) const -> bool;

 private:
    std::vector<std::shared_ptr<ParaDecl>> const paras_;
    std::shared_ptr<CompoundStmt> const stmts_;
    std::string type_output = "";
};

class ConstructorDecl
: public Decl
, public std::enable_shared_from_this<ConstructorDecl> {
 public:
    ConstructorDecl(Position const pos,
                    std::string const ident,
                    std::vector<std::shared_ptr<ParaDecl>> paras,
                    std::shared_ptr<CompoundStmt> stmts)
    : Decl(pos, ident, std::make_shared<Type>())
    , paras_(paras)
    , stmts_(stmts) {}

    auto get_paras() const -> std::vector<std::shared_ptr<ParaDecl>> const& {
        return paras_;
    }
    auto get_compound_stmt() const -> std::shared_ptr<CompoundStmt> const& {
        return stmts_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_constructor_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto operator==(const ConstructorDecl& other) const -> bool;

    auto get_type_output() -> std::string {
        if (type_output.size() != 0) {
            return type_output;
        }

        auto buffer = std::stringstream{};
        for (auto& para : paras_) {
            buffer << *para->get_type();
        }
        buffer << '.';
        type_output = buffer.str();
        return type_output;
    }

 private:
    std::vector<std::shared_ptr<ParaDecl>> const paras_;
    std::shared_ptr<CompoundStmt> const stmts_;
    std::string type_output = "";
};

class DestructorDecl
: public Decl
, public std::enable_shared_from_this<DestructorDecl> {
 public:
    DestructorDecl(Position const pos, std::string const ident, std::shared_ptr<CompoundStmt> stmts)
    : Decl(pos, ident, std::make_shared<Type>(TypeSpec::VOID))
    , stmts_(stmts) {}

    auto get_compound_stmt() const -> std::shared_ptr<CompoundStmt> const& {
        return stmts_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_destructor_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

 private:
    std::shared_ptr<CompoundStmt> const stmts_;
};

class Extern
: public Decl
, public std::enable_shared_from_this<Extern> {
 public:
    Extern(Position pos, std::string const ident, std::shared_ptr<Type> const t, std::vector<std::shared_ptr<Type>> types)
    : Decl(pos, ident, t)
    , types_(types) {}

    auto get_types() const -> std::vector<std::shared_ptr<Type>> {
        return types_;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_extern(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto operator==(const Extern& other) const -> bool;

    auto set_variatic() -> void {
        has_variatic_ = true;
    }

    auto is_variatic() -> bool {
        return has_variatic_;
    }

 private:
    std::vector<std::shared_ptr<Type>> const types_;
    bool has_variatic_ = false;
};

class EnumDecl
: public Decl
, public std::enable_shared_from_this<EnumDecl> {
 public:
    EnumDecl(Position const pos, std::string const name, std::vector<std::string> const fields)
    : Decl(pos, name, std::make_shared<Type>())
    , fields_(fields) {}

    static std::shared_ptr<EnumDecl>
    make(Position const pos, std::string const name, std::vector<std::string> const fields) {
        auto decl = std::make_shared<EnumDecl>(pos, name, fields);
        decl->set_type(std::make_shared<EnumType>(decl));
        return decl;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_enum_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto get_num(std::string field) const -> std::optional<int>;
    auto find_duplicates() const -> std::vector<std::string>;

    auto get_fields() const -> std::vector<std::string> const& {
        return fields_;
    }

 private:
    std::vector<std::string> const fields_;
};

class ClassFieldDecl
: public Decl
, public std::enable_shared_from_this<ClassFieldDecl> {
 public:
    ClassFieldDecl(Position const pos, std::string const name, std::shared_ptr<Type> type)
    : Decl(pos, name, type) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_class_field_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto operator==(const ClassFieldDecl& other) const -> bool;

 private:
};

class ClassDecl
: public Decl
, public std::enable_shared_from_this<ClassDecl> {
 public:
    static std::shared_ptr<ClassDecl> make(Position const pos,
                                           std::string const name,
                                           std::vector<std::shared_ptr<ClassFieldDecl>> fields,
                                           std::vector<std::shared_ptr<MethodDecl>> methods,
                                           std::vector<std::shared_ptr<ConstructorDecl>> constructors,
                                           std::vector<std::shared_ptr<DestructorDecl>> destructors) {
        auto decl = std::shared_ptr<ClassDecl>(new ClassDecl(pos, name, fields, methods, constructors, destructors));
        decl->set_type(std::make_shared<ClassType>(decl));
        return decl;
    }

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_class_decl(shared_from_this());
    }
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override;
    auto print(std::ostream& os) const -> void override;

    auto operator==(const ClassDecl& other) const -> bool;

    auto get_class_type_name() -> std::string;

    auto get_index_for_field(std::string field_name) const -> int;
    auto get_field_type(std::string field_name) const -> std::shared_ptr<Type>;
    auto field_exists(std::string const& name) const -> bool;
    auto method_exists(std::string const& name) const -> bool;
    auto field_is_private(std::string const& name) const -> bool;
    auto get_field(std::string const& name) const -> std::shared_ptr<ClassFieldDecl>;
    auto get_method(std::shared_ptr<MethodAccessExpr> method) const -> std::optional<std::shared_ptr<MethodDecl>>;

    auto get_fields() const -> std::vector<std::shared_ptr<ClassFieldDecl>> {
        return fields_;
    }

    auto get_methods() const -> std::vector<std::shared_ptr<MethodDecl>> {
        return methods_;
    }

    auto get_constructors() const -> std::vector<std::shared_ptr<ConstructorDecl>> {
        return constructors_;
    }

    auto get_destructors() const -> std::vector<std::shared_ptr<DestructorDecl>> {
        return destructors_;
    }

 private:
    ClassDecl(Position const pos, std::string const name)
    : Decl(pos, name, std::make_shared<Type>()) {}

    ClassDecl(Position const pos,
              std::string const name,
              std::vector<std::shared_ptr<ClassFieldDecl>> fields,
              std::vector<std::shared_ptr<MethodDecl>> methods,
              std::vector<std::shared_ptr<ConstructorDecl>> constructors,
              std::vector<std::shared_ptr<DestructorDecl>> destructors)
    : Decl(pos, name, std::make_shared<Type>())
    , fields_(fields)
    , methods_(methods)
    , constructors_(constructors)
    , destructors_(destructors) {}

    std::string type_name_ = {};
    std::vector<std::shared_ptr<ClassFieldDecl>> fields_;
    std::vector<std::shared_ptr<MethodDecl>> methods_;
    std::vector<std::shared_ptr<ConstructorDecl>> constructors_;
    std::vector<std::shared_ptr<DestructorDecl>> destructors_;
};

#endif // DECL_HPP