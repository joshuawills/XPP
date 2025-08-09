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
    Decl(Position pos, std::string ident, Type t)
    : AST(pos)
    , ident_(std::move(ident))
    , t_(std::move(t)) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override = 0;
    auto codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* override = 0;
    auto print(std::ostream& os) const -> void override = 0;

    auto set_type(Type t) -> void {
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

    auto is_exported() const -> bool {
        return is_exported_;
    }
    auto set_exported() -> void {
        is_exported_ = true;
    }

    auto get_ident() const -> std::string const& {
        return ident_;
    }
    auto get_type() const -> Type {
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
    bool is_used_ = false, is_reassigned_ = false, is_mut_ = false, is_exported_ = false;
    std::string ident_;
    Type t_;
    size_t statement_num_ = 0, depth_num_ = 0;
};

class ParaDecl
: public Decl
, public std::enable_shared_from_this<ParaDecl> {
 public:
    ParaDecl(Position pos, std::string ident, Type t)
    : Decl(pos, ident, t) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_para_decl(shared_from_this());
    }

    auto operator==(const ParaDecl& other) const -> bool {
        return t_ == other.t_;
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
    LocalVarDecl(Position pos, std::string ident, Type t, std::shared_ptr<Expr> e)
    : Decl(pos, ident, t)
    , expr_(e) {}

    auto visit(std::shared_ptr<Visitor> visitor) -> void override {
        visitor->visit_local_var_decl(shared_from_this());
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
    GlobalVarDecl(Position pos, std::string const ident, Type const t, std::shared_ptr<Expr> const expr)
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

    auto get_expr() const -> std::shared_ptr<Expr> {
        return expr_;
    }

 private:
    std::shared_ptr<Expr> const expr_;
};

class Function
: public Decl
, public std::enable_shared_from_this<Function> {
 public:
    Function(Position const pos,
             std::string const ident,
             std::vector<std::shared_ptr<ParaDecl>> paras,
             Type t,
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
            buffer << para->get_type();
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

class Extern
: public Decl
, public std::enable_shared_from_this<Extern> {
 public:
    Extern(Position pos, std::string const ident, Type const t, std::vector<Type> types)
    : Decl(pos, ident, t)
    , types_(types) {}

    auto get_types() const -> std::vector<Type> {
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
    std::vector<Type> const types_;
    bool has_variatic_ = false;
};

#endif // DECL_HPP