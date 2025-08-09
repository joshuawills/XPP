#ifndef PARSER_HPP
#define PARSER_HPP

#include "./handler.hpp"
#include "./module.hpp"
#include "./token.hpp"
#include <vector>

class Parser {
 public:
    Parser(std::vector<std::shared_ptr<Token>>& tokens,
           std::string const& filename,
           std::shared_ptr<Handler> handler = nullptr)
    : tokens_{tokens}
    , filename_{filename}
    , handler_{handler} {
        if (!tokens.empty()) {
            curr_token_ = tokens_.front();
        }
    }

    ~Parser() = default;

    auto parse() -> std::shared_ptr<Module>;

 private:
    std::vector<std::shared_ptr<Token>>& tokens_;
    std::string const& filename_;
    std::shared_ptr<Handler> handler_;
    std::optional<std::shared_ptr<Token>> curr_token_ = std::nullopt;
    std::size_t index = 0;

    auto try_consume(TokenType t) -> bool;
    auto consume() -> void;
    auto match(TokenType t) -> void;
    auto start(Position& pos) -> void;
    auto finish(Position& pos) -> void;
    auto peek(TokenType t, size_t pos = 0) -> bool;

    auto parse_operator() -> Op;
    auto parse_ident() -> std::string;
    auto parse_type() -> std::shared_ptr<Type>;
    auto parse_para_list() -> std::vector<std::shared_ptr<ParaDecl>>;
    auto parse_type_list() -> std::vector<std::shared_ptr<Type>>;
    auto parse_arg_list() -> std::vector<std::shared_ptr<Expr>>;

    auto parse_compound_stmt() -> std::shared_ptr<CompoundStmt>;
    auto parse_local_var_stmt() -> std::shared_ptr<LocalVarStmt>;
    auto parse_return_stmt(Position p) -> std::shared_ptr<ReturnStmt>;
    auto parse_while_stmt(Position p) -> std::shared_ptr<WhileStmt>;
    auto parse_if_stmt(Position p) -> std::shared_ptr<IfStmt>;
    auto parse_else_if_stmt(Position p) -> std::shared_ptr<ElseIfStmt>;
    auto parse_expr_stmt(Position p) -> std::shared_ptr<ExprStmt>;

    auto parse_expr() -> std::shared_ptr<Expr>;
    auto parse_assignment_expr() -> std::shared_ptr<Expr>;
    auto parse_logical_or_expr() -> std::shared_ptr<Expr>;
    auto parse_logical_and_expr() -> std::shared_ptr<Expr>;
    auto parse_equality_expr() -> std::shared_ptr<Expr>;
    auto parse_relational_expr() -> std::shared_ptr<Expr>;
    auto parse_additive_expr() -> std::shared_ptr<Expr>;
    auto parse_multiplicative_expr() -> std::shared_ptr<Expr>;
    auto parse_unary_expr() -> std::shared_ptr<Expr>;
    auto parse_postfix_expr() -> std::shared_ptr<Expr>;
    auto parse_primary_expr() -> std::shared_ptr<Expr>;

    auto is_assignment_operator() -> bool;

    auto syntactic_error(const std::string& _template, const std::string& quoted_token) -> void;
};

#endif // PARSER_HPP