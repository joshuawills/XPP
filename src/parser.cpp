#include "./parser.hpp"
#include <iostream>
#include <map>

auto Parser::syntactic_error(const std::string& _template, const std::string& quoted_token) -> void {
    handler_->report_error(filename_, _template, quoted_token, (*curr_token_)->pos());
    exit(EXIT_FAILURE);
}

auto Parser::start(Position& pos) -> void {
    if (curr_token_.has_value()) {
        pos = (*curr_token_)->pos();
    }
}

auto Parser::finish(Position& pos) -> void {
    if (curr_token_.has_value()) {
        pos = (*curr_token_)->pos();
    }
}

auto Parser::try_consume(TokenType t) -> bool {
    if (!curr_token_.has_value()) {
        return false;
    }

    if ((*curr_token_)->type_matches(t)) {
        consume();
        return true;
    }

    return false;
}

auto Parser::match(TokenType t) -> void {
    if (!try_consume(t)) {
        std::cout << "Received: " << token_type_to_str((*curr_token_)->type()) << "\n";
        syntactic_error("\"%\" expected here", token_type_to_str(t));
    }
}

auto Parser::consume() -> void {
    ++index;
    if (index < tokens_.size()) {
        curr_token_ = tokens_[index];
    }
    else {
        curr_token_ = std::nullopt;
    }
}

auto Parser::peek(TokenType t, size_t pos) -> bool {
    if (index + pos >= tokens_.size()) {
        return false;
    }
    return tokens_[index + pos]->type_matches(t);
}

auto Parser::parse() -> std::shared_ptr<Module> {
    auto module = std::make_shared<Module>(filename_);

    while (curr_token_.has_value()) {
        auto p = Position{};
        start(p);
        if (try_consume(TokenType::FN)) {
            auto ident = parse_ident();
            auto paras = parse_para_list();
            auto type = parse_type();
            auto stmt = parse_compound_stmt();
            finish(p);
            auto func = std::make_shared<Function>(p, ident, paras, type, stmt);
            module->add_function(func);
        }
    }

    return module;
}

auto Parser::parse_operator() -> Operator {
    if (!curr_token_.has_value()) {
        syntactic_error("OPERATOR expected, but found end of file", "");
    }

    auto const type_to_operator_mapping = std::map<TokenType, Operator>{{TokenType::ASSIGN, Operator::ASSIGN},
                                                                        {TokenType::LOGICAL_OR, Operator::LOGICAL_OR},
                                                                        {TokenType::LOGICAL_AND, Operator::LOGICAL_AND},
                                                                        {TokenType::EQUAL, Operator::EQUAL},
                                                                        {TokenType::NOT_EQUAL, Operator::NOT_EQUAL},
                                                                        {TokenType::NEGATE, Operator::NEGATE},
                                                                        {TokenType::PLUS, Operator::PLUS},
                                                                        {TokenType::MINUS, Operator::MINUS},
                                                                        {TokenType::MULTIPLY, Operator::MULTIPLY},
                                                                        {TokenType::DIVIDE, Operator::DIVIDE}};

    if (type_to_operator_mapping.find((*curr_token_)->type()) != type_to_operator_mapping.end()) {
        auto op = type_to_operator_mapping.at((*curr_token_)->type());
        consume();
        return op;
    }

    syntactic_error("UNRECOGNIZED OPERATOR: %", (*curr_token_)->str());
    return Operator::ASSIGN;
}

auto Parser::parse_ident() -> std::string {
    if (!curr_token_.has_value()) {
        syntactic_error("IDENTIFIER expected, but found end of file", "");
    }
    auto spelling = (*curr_token_)->str();
    match(TokenType::IDENT);
    return spelling;
}

auto Parser::parse_type() -> Type {
    if (!curr_token_.has_value()) {
        syntactic_error("TYPE expected, but found end of file", "");
    }
    auto pos = Position{};
    start(pos);

    auto curr_lexeme = (*curr_token_)->lexeme();
    auto type_spec = type_spec_from_lexeme(curr_lexeme);
    if (type_spec.has_value()) {
        consume();
        return Type{*type_spec, std::nullopt};
    }

    syntactic_error("TYPE expected, but found \"%\"", curr_lexeme);
    return Type{TypeSpec::UNKNOWN, std::nullopt};
}

auto Parser::parse_para_list() -> std::vector<std::shared_ptr<ParaDecl>> {
    std::vector<std::shared_ptr<ParaDecl>> paras;

    match(TokenType::OPEN_BRACKET);
    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_BRACKET)) {
        auto p = Position{};
        start(p);
        auto const is_mut = try_consume(TokenType::MUT);
        auto ident = parse_ident();
        match(TokenType::COLON);
        auto type = parse_type();
        finish(p);
        auto decl = std::make_shared<ParaDecl>(p, ident, type);
        if (is_mut) {
            decl->set_mut();
        }
        paras.push_back(decl);
        if (peek(TokenType::CLOSE_BRACKET)) {
            break;
        }
        match(TokenType::COMMA);
    }
    match(TokenType::CLOSE_BRACKET);
    return paras;
}

auto Parser::parse_compound_stmt() -> std::vector<std::shared_ptr<Stmt>> {
    auto stmts = std::vector<std::shared_ptr<Stmt>>{};
    match(TokenType::OPEN_CURLY);
    if (try_consume(TokenType::CLOSE_CURLY)) {
        return stmts;
    }

    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_CURLY)) {
        auto p = Position{};
        start(p);
        if (try_consume(TokenType::SEMICOLON)) {
            finish(p);
            stmts.push_back(std::make_shared<EmptyStmt>(p));
        }
        else if (try_consume(TokenType::LET)) {
            auto const is_mut = try_consume(TokenType::MUT);
            auto ident = parse_ident();
            auto t = Type{TypeSpec::UNKNOWN, std::nullopt};
            if (try_consume(TokenType::COLON)) {
                t = parse_type();
            }
            finish(p);
            std::shared_ptr<Expr> e = std::make_shared<EmptyExpr>(p);
            if (try_consume(TokenType::ASSIGN)) {
                e = parse_expr();
            }
            match(TokenType::SEMICOLON);
            std::shared_ptr<LocalVarDecl> decl = std::make_shared<LocalVarDecl>(p, ident, t, e);
            if (is_mut) {
                decl->set_mut();
            }
            stmts.push_back(std::make_shared<LocalVarStmt>(p, decl));
        }
        else {
            syntactic_error("UNRECOGNIZED STATEMENT: %", (*curr_token_)->str());
        }
    }
    match(TokenType::CLOSE_CURLY);

    return stmts;
}

auto Parser::parse_expr() -> std::shared_ptr<Expr> {
    return parse_assignment_expr();
}

auto Parser::parse_assignment_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    auto left = parse_logical_or_expr();
    if (!is_assignment_operator()) {
        return left;
    }
    auto op = parse_operator();
    auto right = parse_assignment_expr();
    finish(p);
    return std::make_shared<AssignmentExpr>(p, left, op, right);
}

auto Parser::parse_logical_or_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_logical_and_expr();
    while (peek(TokenType::LOGICAL_OR)) {
        auto op = parse_operator();
        auto right = parse_logical_and_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_logical_and_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_equality_expr();
    while (peek(TokenType::LOGICAL_AND)) {
        auto op = parse_operator();
        auto right = parse_equality_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_equality_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_relational_expr();
    while (peek(TokenType::EQUAL) or peek(TokenType::NOT_EQUAL)) {
        auto op = parse_operator();
        auto right = parse_relational_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_relational_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_additive_expr();
    while (peek(TokenType::LESS_THAN) or peek(TokenType::LESS_EQUAL) or peek(TokenType::GREATER_THAN)
           or peek(TokenType::GREATER_EQUAL))
    {
        auto op = parse_operator();
        auto right = parse_additive_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_additive_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_multiplicative_expr();
    while (peek(TokenType::PLUS) or peek(TokenType::MINUS)) {
        auto op = parse_operator();
        auto right = parse_multiplicative_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_multiplicative_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    std::shared_ptr<Expr> left = parse_unary_expr();
    while (peek(TokenType::MULTIPLY) or peek(TokenType::DIVIDE)) {
        auto op = parse_operator();
        auto right = parse_unary_expr();
        finish(p);
        left = std::make_shared<BinaryExpr>(p, left, op, right);
    }
    return left;
}

auto Parser::parse_unary_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);

    if (peek(TokenType::NEGATE) or peek(TokenType::PLUS) or peek(TokenType::MINUS)) {
        auto op = parse_operator();
        auto expr = parse_unary_expr();
        finish(p);
        return std::make_shared<UnaryExpr>(p, op, expr);
    }
    else {
        return parse_primary_expr();
    }
}

auto Parser::parse_primary_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    if (peek(TokenType::IDENT)) {
        auto const value = parse_ident();
        finish(p);
        return std::make_shared<VarExpr>(p, value);
    }
    else if (peek(TokenType::INTEGER)) {
        auto const value = std::stoi((*curr_token_)->lexeme());
        consume();
        finish(p);
        return std::make_shared<IntExpr>(p, value);
    }
    else if (peek(TokenType::OPEN_BRACKET)) {
        match(TokenType::OPEN_BRACKET);
        auto expr = parse_expr();
        match(TokenType::CLOSE_BRACKET);
        return expr;
    }

    syntactic_error("UNRECOGNIZED PRIMARY EXPRESSION: %", (*curr_token_)->str());
    return std::make_shared<EmptyExpr>(p);
}

auto Parser::is_assignment_operator() -> bool {
    return curr_token_.has_value() and (*curr_token_)->type_matches(TokenType::ASSIGN);
}