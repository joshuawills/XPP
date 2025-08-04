#ifndef DECL_HPP
#define DECL_HPP

#include "./ast.hpp"
#include "./expr.hpp"
#include "./stmt.hpp"
#include "./type.hpp"

class Decl : public AST {
 public:
    Decl(Position pos, std::string ident, Type t)
    : AST(pos)
    , ident_(std::move(ident))
    , t_(std::move(t)) {}

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
    auto get_type() const -> Type const& {
        return t_;
    }

 private:
    bool is_used_ = false, is_reassigned_ = false, is_mut_ = false, is_exported_ = false;
    std::string ident_;
    Type t_;
};

class ParaDecl : public Decl {
 public:
    ParaDecl(Position pos, std::string ident, Type t)
    : Decl(pos, ident, t) {}

 private:
};

class LocalVarDecl : public Decl {
 public:
    LocalVarDecl(Position pos, std::string ident, Type t, std::shared_ptr<Expr> e)
    : Decl(pos, ident, t)
    , e_(e) {}

 private:
    std::shared_ptr<Expr> e_;
};

class Function : public Decl {
 public:
    Function(Position pos,
             std::string ident,
             std::vector<std::shared_ptr<ParaDecl>> paras,
             Type t,
             std::vector<std::shared_ptr<Stmt>> stmts)
    : Decl(pos, ident, t)
    , paras_(std::move(paras))
    , stmts_(std::move(stmts)) {}

    auto get_paras() const -> std::vector<std::shared_ptr<ParaDecl>> const& {
        return paras_;
    }
    auto get_stmts() const -> std::vector<std::shared_ptr<Stmt>> const& {
        return stmts_;
    }

 private:
    std::vector<std::shared_ptr<ParaDecl>> paras_;
    std::vector<std::shared_ptr<Stmt>> stmts_;
};

#endif // DECL_HPP