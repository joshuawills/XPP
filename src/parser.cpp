#include "./parser.hpp"
#include <iostream>
#include <map>
#include <sstream>

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
        pos.col_end_ = (*curr_token_)->pos().col_end_;
        pos.line_end_ = (*curr_token_)->pos().line_end_;
    }
    else {
        pos.col_end_ = tokens_.back()->pos().col_end_;
        pos.line_end_ = tokens_.back()->pos().line_end_;
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
        std::cout << "Received: " << (*curr_token_)->type() << "\n";
        auto stream = std::stringstream{};
        stream << t;
        syntactic_error("\"%\" expected here", stream.str());
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
        auto const is_pub = try_consume(TokenType::PUB);
        if (try_consume(TokenType::FN)) {
            auto ident = parse_ident();
            auto paras = parse_para_list();
            auto type = parse_type();
            auto stmt = parse_compound_stmt();
            finish(p);
            auto func = std::make_shared<Function>(p, ident, paras, type, stmt);
            if (is_pub)
                func->set_pub();
            stmt->set_parent(func);
            module->add_function(func);
        }
        else if (try_consume(TokenType::EXTERN)) {
            auto ident = parse_ident();
            auto types = parse_type_list();
            auto return_type = parse_type();
            match(TokenType::SEMICOLON);
            finish(p);
            auto extern_ = std::make_shared<Extern>(p, ident, return_type, types);
            if (is_pub)
                extern_->set_pub();
            module->add_extern(extern_);
        }
        else if (try_consume(TokenType::ENUM)) {
            auto ident = parse_ident();
            auto enum_list = parse_enum_list();
            finish(p);
            auto enum_ = EnumDecl::make(p, ident, enum_list);
            if (is_pub)
                enum_->set_pub();
            module->add_enums(enum_);
        }
        else if (try_consume(TokenType::LET)) {
            auto const is_mut = try_consume(TokenType::MUT);
            auto const ident = parse_ident();
            auto type = std::make_shared<Type>();
            if (try_consume(TokenType::COLON)) {
                type = parse_type();
            }
            finish(p);
            std::shared_ptr<Expr> expr = std::make_shared<EmptyExpr>(p);
            if (try_consume(TokenType::ASSIGN)) {
                expr = parse_expr();
            }
            auto global_var = std::make_shared<GlobalVarDecl>(p, ident, type, expr);
            expr->set_parent(global_var);
            if (is_mut) {
                global_var->set_mut();
            }
            if (is_pub)
                global_var->set_pub();
            module->add_global_var(global_var);
            match(TokenType::SEMICOLON);
        }
        else if (try_consume(TokenType::CLASS)) {
            auto class_ = parse_class(p);
            if (is_pub)
                class_->set_pub();
            module->add_class(class_);
        }
        else {
            auto stream = std::stringstream{};
            stream << *(*curr_token_);
            syntactic_error("Expected a type declaration, function declaration or global varariable declaration, "
                            "received %",
                            stream.str());
        }
    }

    return module;
}

auto Parser::parse_class(Position p) -> std::shared_ptr<ClassDecl> {
    auto const class_name = parse_ident();
    auto fields_vec = std::vector<std::shared_ptr<ClassFieldDecl>>{};
    auto methods_vec = std::vector<std::shared_ptr<MethodDecl>>{};
    auto constructors_vec = std::vector<std::shared_ptr<ConstructorDecl>>{};

    match(TokenType::OPEN_CURLY);
    while (!peek(TokenType::CLOSE_CURLY)) {
        auto p2 = Position{};
        start(p2);
        auto const is_pub = try_consume(TokenType::PUB);
        auto const is_mut = try_consume(TokenType::MUT);
        if (peek(TokenType::IDENT) and (*curr_token_)->lexeme() != class_name) {
            auto lex = (*curr_token_)->lexeme();
            match(TokenType::IDENT);
            if (try_consume(TokenType::COLON)) {
                auto type = parse_type();
                match(TokenType::SEMICOLON);
                finish(p2);
                auto field_decl = std::make_shared<ClassFieldDecl>(p2, lex, type);
                if (is_pub) {
                    field_decl->set_pub();
                }
                if (is_mut) {
                    field_decl->set_mut();
                }
                fields_vec.push_back(field_decl);
            }
            else {
                std::cout << "Parser::parse_class UNREACHABLE!\n";
            }
        }
        else if (peek(TokenType::IDENT) and (*curr_token_)->lexeme() == class_name) { // constructor
            consume();
            auto paras = parse_para_list();
            auto stmts = parse_compound_stmt();
            finish(p2);
            auto constructor_decl = std::make_shared<ConstructorDecl>(p2, class_name, paras, stmts);
            if (is_pub) {
                constructor_decl->set_pub();
            }
            constructors_vec.push_back(constructor_decl);
        }
        else if (try_consume(TokenType::FN)) {
            auto const ident = parse_ident();
            auto paras = parse_para_list();
            auto type = parse_type();
            auto stmts = parse_compound_stmt();
            finish(p2);
            auto method_decl = std::make_shared<MethodDecl>(p2, ident, paras, type, stmts);
            if (is_pub) {
                method_decl->set_pub();
            }
            if (is_mut) {
                method_decl->set_mut();
            }
            methods_vec.push_back(method_decl);
        }
        else {
            std::cout << "Parser::parse_class UNREACHABLE!\n";
        }
    }
    match(TokenType::CLOSE_CURLY);

    finish(p);
    auto class_ = ClassDecl::make(p, class_name, fields_vec, methods_vec, constructors_vec);
    return class_;
}

auto Parser::parse_operator() -> Op {
    if (!curr_token_.has_value()) {
        syntactic_error("OPERATOR expected, but found end of file", "");
    }

    auto const type_to_operator_mapping = std::map<TokenType, Op>{{TokenType::ASSIGN, Op::ASSIGN},
                                                                  {TokenType::LOGICAL_OR, Op::LOGICAL_OR},
                                                                  {TokenType::LOGICAL_AND, Op::LOGICAL_AND},
                                                                  {TokenType::EQUAL, Op::EQUAL},
                                                                  {TokenType::NOT_EQUAL, Op::NOT_EQUAL},
                                                                  {TokenType::NEGATE, Op::NEGATE},
                                                                  {TokenType::PLUS, Op::PLUS},
                                                                  {TokenType::MINUS, Op::MINUS},
                                                                  {TokenType::MULTIPLY, Op::MULTIPLY},
                                                                  {TokenType::DIVIDE, Op::DIVIDE},
                                                                  {TokenType::LESS_THAN, Op::LESS_THAN},
                                                                  {TokenType::GREATER_THAN, Op::GREATER_THAN},
                                                                  {TokenType::LESS_EQUAL, Op::LESS_EQUAL},
                                                                  {TokenType::AMPERSAND, Op::ADDRESS_OF},
                                                                  {TokenType::MODULO, Op::MODULO},
                                                                  {TokenType::PLUS_ASSIGN, Op::PLUS_ASSIGN},
                                                                  {TokenType::MINUS_ASSIGN, Op::MINUS_ASSIGN},
                                                                  {TokenType::MULTIPLY_ASSIGN, Op::MULTIPLY_ASSIGN},
                                                                  {TokenType::DIVIDE_ASSIGN, Op::DIVIDE_ASSIGN},
                                                                  {TokenType::GREATER_EQUAL, Op::GREATER_EQUAL}};

    if (type_to_operator_mapping.find((*curr_token_)->type()) != type_to_operator_mapping.end()) {
        auto const& op = type_to_operator_mapping.at((*curr_token_)->type());
        consume();
        return op;
    }

    auto stream = std::stringstream{};
    stream << *curr_token_;
    syntactic_error("UNRECOGNIZED OPERATOR: %", stream.str());
    return Op::ASSIGN;
}

auto Parser::parse_ident() -> std::string {
    if (!curr_token_.has_value()) {
        syntactic_error("IDENTIFIER expected, but found end of file", "");
    }
    auto const& spelling = (*curr_token_)->lexeme();
    match(TokenType::IDENT);
    return spelling;
}

auto Parser::parse_type() -> std::shared_ptr<Type> {
    if (!curr_token_.has_value()) {
        syntactic_error("TYPE expected, but found end of file", "");
    }

    auto const& curr_lexeme = (*curr_token_)->lexeme();
    auto const& type_spec = type_spec_from_lexeme(curr_lexeme);

    consume();

    // Handle pointer/array types
    std::shared_ptr<Type> return_type;
    if (type_spec == TypeSpec::MURKY) {
        return_type = std::make_shared<MurkyType>(curr_lexeme);
    }
    else {
        return_type = std::make_shared<Type>(type_spec);
    }
    std::shared_ptr<Type> sub_type = nullptr;
    if (try_consume(TokenType::OPEN_SQUARE)) {
        sub_type = return_type;
        if (peek(TokenType::INTEGER)) {
            auto value = std::stoul((*curr_token_)->lexeme());
            consume();
            match(TokenType::CLOSE_SQUARE);
            return std::make_shared<ArrayType>(sub_type, size_t{value});
        }
        else {
            match(TokenType::CLOSE_SQUARE);
            return std::make_shared<ArrayType>(sub_type);
        }
        match(TokenType::CLOSE_SQUARE);
    }
    else {
        while (try_consume(TokenType::MULTIPLY)) {
            sub_type = return_type;
            return_type = std::make_shared<PointerType>(sub_type);
        }
    }

    return return_type;
}

auto Parser::parse_para_list() -> std::vector<std::shared_ptr<ParaDecl>> {
    auto paras = std::vector<std::shared_ptr<ParaDecl>>{};

    match(TokenType::OPEN_BRACKET);
    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_BRACKET)) {
        auto p = Position{};
        start(p);
        auto const is_mut = try_consume(TokenType::MUT);
        auto const ident = parse_ident();
        match(TokenType::COLON);
        auto const type = parse_type();
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

auto Parser::parse_type_list() -> std::vector<std::shared_ptr<Type>> {
    auto types = std::vector<std::shared_ptr<Type>>{};
    match(TokenType::OPEN_BRACKET);
    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_BRACKET)) {
        types.push_back(parse_type());
        if (peek(TokenType::CLOSE_BRACKET)) {
            break;
        }
        match(TokenType::COMMA);
    }
    match(TokenType::CLOSE_BRACKET);
    return types;
}

auto Parser::parse_arg_list() -> std::vector<std::shared_ptr<Expr>> {
    auto args = std::vector<std::shared_ptr<Expr>>{};
    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_BRACKET)) {
        args.push_back(parse_expr());
        if (peek(TokenType::CLOSE_BRACKET)) {
            break;
        }
        match(TokenType::COMMA);
    }
    match(TokenType::CLOSE_BRACKET);
    return args;
}

auto Parser::parse_enum_list() -> std::vector<std::string> {
    auto res = std::vector<std::string>{};
    match(TokenType::OPEN_CURLY);
    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_CURLY)) {
        auto s = (*curr_token_)->lexeme();
        match(TokenType::IDENT);
        res.push_back(s);
        if (peek(TokenType::CLOSE_CURLY)) {
            break;
        }
        match(TokenType::COMMA);
    }
    match(TokenType::CLOSE_CURLY);
    return res;
}

auto Parser::parse_compound_stmt() -> std::shared_ptr<CompoundStmt> {
    auto stmts = std::vector<std::shared_ptr<Stmt>>{};
    match(TokenType::OPEN_CURLY);

    auto p = Position{};
    start(p);
    if (try_consume(TokenType::CLOSE_CURLY)) {
        return std::make_shared<CompoundStmt>(p, stmts);
    }

    while (curr_token_.has_value() and !(*curr_token_)->type_matches(TokenType::CLOSE_CURLY)) {
        if (try_consume(TokenType::SEMICOLON)) {
            finish(p);
            stmts.push_back(std::make_shared<EmptyStmt>(p));
        }
        else if (try_consume(TokenType::LET)) {
            stmts.push_back(parse_local_var_stmt());
        }
        else if (try_consume(TokenType::RETURN)) {
            stmts.push_back(parse_return_stmt(p));
        }
        else if (try_consume(TokenType::WHILE)) {
            stmts.push_back(parse_while_stmt(p));
        }
        else if (try_consume(TokenType::IF)) {
            stmts.push_back(parse_if_stmt(p));
        }
        else if (peek(TokenType::OPEN_CURLY)) {
            stmts.push_back(parse_compound_stmt());
        }
        else {
            stmts.push_back(parse_expr_stmt(p));
        }
    }
    match(TokenType::CLOSE_CURLY);

    finish(p);
    return std::make_shared<CompoundStmt>(p, stmts);
}

auto Parser::parse_local_var_stmt() -> std::shared_ptr<LocalVarStmt> {
    Position p;
    start(p);
    auto const is_mut = try_consume(TokenType::MUT);
    auto ident = parse_ident();
    auto t = std::make_shared<Type>();
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
    e->set_parent(decl);
    if (is_mut) {
        decl->set_mut();
    }
    finish(p);
    return std::make_shared<LocalVarStmt>(p, decl);
}
auto Parser::parse_return_stmt(Position p) -> std::shared_ptr<ReturnStmt> {
    std::shared_ptr<Expr> expr;
    if (try_consume(TokenType::SEMICOLON)) {
        finish(p);
        expr = std::make_shared<EmptyExpr>(p);
    }
    else {
        expr = parse_expr();
        match(TokenType::SEMICOLON);
    }
    finish(p);
    return std::make_shared<ReturnStmt>(p, expr);
}

auto Parser::parse_while_stmt(Position p) -> std::shared_ptr<WhileStmt> {
    auto const cond = parse_expr();
    auto const stmts_ = parse_compound_stmt();
    finish(p);
    auto while_stmt = std::make_shared<WhileStmt>(p, cond, stmts_);
    stmts_->set_parent(while_stmt);
    return while_stmt;
}

auto Parser::parse_if_stmt(Position p) -> std::shared_ptr<IfStmt> {
    auto const cond = parse_expr();
    auto const stmt_one = parse_compound_stmt();
    auto else_if_p = p;
    finish(p);
    std::shared_ptr<Stmt> stmt_two = std::make_shared<EmptyStmt>(p);
    if (try_consume(TokenType::ELSE_IF)) {
        stmt_two = parse_else_if_stmt(else_if_p);
    }

    finish(p);
    std::shared_ptr<Stmt> stmt_three = std::make_shared<EmptyStmt>(p);
    if (try_consume(TokenType::ELSE)) {
        stmt_three = parse_compound_stmt();
    }

    finish(p);
    return std::make_shared<IfStmt>(p, cond, stmt_one, stmt_two, stmt_three);
}

auto Parser::parse_else_if_stmt(Position p) -> std::shared_ptr<ElseIfStmt> {
    auto const cond = parse_expr();
    auto const stmt = parse_compound_stmt();
    auto else_p = p;
    finish(p);
    std::shared_ptr<Stmt> stmt_two = std::make_shared<EmptyStmt>(p);
    if (try_consume(TokenType::ELSE_IF)) {
        stmt_two = parse_else_if_stmt(else_p);
    }
    finish(p);
    return std::make_shared<ElseIfStmt>(p, cond, stmt, stmt_two);
}

auto Parser::parse_expr_stmt(Position p) -> std::shared_ptr<ExprStmt> {
    auto const expr = parse_expr();
    match(TokenType::SEMICOLON);
    finish(p);
    return std::make_shared<ExprStmt>(p, expr);
}

auto Parser::parse_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    auto expr = parse_assignment_expr();
    if (try_consume(TokenType::AS)) {
        auto const type = parse_type();
        finish(p);
        return std::make_shared<CastExpr>(p, expr, type);
    }
    return expr;
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
    auto assignment_expr = std::make_shared<AssignmentExpr>(p, left, op, right);
    left->set_parent(assignment_expr);
    return assignment_expr;
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
    while (peek(TokenType::MULTIPLY) or peek(TokenType::DIVIDE) or peek(TokenType::MODULO)) {
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

    if (peek(TokenType::PLUS_PLUS) or peek(TokenType::MINUS_MINUS)) {
        auto const is_plus = peek(TokenType::PLUS_PLUS);
        consume();
        auto const expr = parse_unary_expr();
        finish(p);
        return std::make_shared<UnaryExpr>(p, is_plus ? Op::PREFIX_ADD : Op::PREFIX_MINUS, expr);
    }
    else if (peek(TokenType::NEGATE) or peek(TokenType::PLUS) or peek(TokenType::MINUS) or peek(TokenType::MULTIPLY)
             or peek(TokenType::AMPERSAND))
    {
        auto op = parse_operator();
        if (op == Op::MULTIPLY) {
            op = Op::DEREF;
        }
        auto expr = parse_unary_expr();
        finish(p);
        return std::make_shared<UnaryExpr>(p, op, expr);
    }
    else if (peek(TokenType::OPEN_SQUARE)) {
        return parse_array_init_expr();
    }
    else {
        return parse_postfix_expr();
    }
}

auto Parser::parse_array_init_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    auto exprs = std::vector<std::shared_ptr<Expr>>{};
    match(TokenType::OPEN_SQUARE);
    while (curr_token_.has_value() and !peek(TokenType::CLOSE_SQUARE)) {
        exprs.push_back(parse_expr());
        if (peek(TokenType::CLOSE_SQUARE)) {
            break;
        }
        match(TokenType::COMMA);
    }
    match(TokenType::CLOSE_SQUARE);
    finish(p);
    return std::make_shared<ArrayInitExpr>(p, exprs);
}

auto Parser::parse_postfix_expr() -> std::shared_ptr<Expr> {
    auto p = Position{};
    start(p);
    auto p_expr = parse_primary_expr();
    auto v = std::dynamic_pointer_cast<VarExpr>(p_expr);
    if (peek(TokenType::OPEN_BRACKET) and v) {
        match(TokenType::OPEN_BRACKET);
        auto args = parse_arg_list();
        finish(p);
        return std::make_shared<CallExpr>(p, v->get_name(), args);
    }
    else if (peek(TokenType::PLUS_PLUS) or peek(TokenType::MINUS_MINUS)) {
        auto const is_plus = peek(TokenType::PLUS_PLUS);
        consume();
        finish(p);
        return std::make_shared<UnaryExpr>(p, (is_plus) ? Op::POSTFIX_ADD : Op::POSTFIX_MINUS, p_expr);
    }
    else if (peek(TokenType::OPEN_SQUARE)) {
        match(TokenType::OPEN_SQUARE);
        auto const index_expr = parse_expr();
        match(TokenType::CLOSE_SQUARE);
        finish(p);
        return std::make_shared<ArrayIndexExpr>(p, p_expr, index_expr);
    }
    else if (peek(TokenType::DOUBLE_COLON) and v) {
        auto enum_name = v->get_name();
        match(TokenType::DOUBLE_COLON);
        auto field_name = parse_ident();
        finish(p);
        return std::make_shared<EnumAccessExpr>(p, enum_name, field_name);
    }
    else if (try_consume(TokenType::DOT)) {
        auto field_name = parse_ident();
        if (!peek(TokenType::OPEN_BRACKET)) {
            finish(p);
            return std::make_shared<FieldAccessExpr>(p, p_expr, field_name);
        }
        else {
            match(TokenType::OPEN_BRACKET);
            auto args = parse_arg_list();
            finish(p);
            return std::make_shared<MethodAccessExpr>(p, p_expr, field_name, args);
        }
    }
    else {
        return p_expr;
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
        auto const value = std::stoll((*curr_token_)->lexeme());
        consume();
        finish(p);
        return std::make_shared<IntExpr>(p, value);
    }
    else if (peek(TokenType::FLOAT_LITERAL)) {
        auto const value = std::stod((*curr_token_)->lexeme());
        consume();
        finish(p);
        return std::make_shared<DecimalExpr>(p, value);
    }
    else if (peek(TokenType::UNSIGNED_INTEGER)) {
        auto const value = std::stoull((*curr_token_)->lexeme());
        consume();
        finish(p);
        return std::make_shared<UIntExpr>(p, value);
    }
    else if (peek(TokenType::OPEN_BRACKET)) {
        match(TokenType::OPEN_BRACKET);
        auto expr = parse_expr();
        match(TokenType::CLOSE_BRACKET);
        return expr;
    }
    else if (peek(TokenType::TRUE) or peek(TokenType::FALSE)) {
        auto const value = (*curr_token_)->type_matches(TokenType::TRUE);
        consume();
        finish(p);
        return std::make_shared<BoolExpr>(p, value);
    }
    else if (peek(TokenType::STRING_LITERAL)) {
        auto const value = (*curr_token_)->lexeme();
        consume();
        finish(p);
        return std::make_shared<StringExpr>(p, value);
    }
    else if (peek(TokenType::CHAR_LITERAL)) {
        auto const value = (*curr_token_)->lexeme();
        if (value.size() > 1) {
            syntactic_error("18: character literal may only have one character: '%'", value);
        }
        consume();
        finish(p);
        return std::make_shared<CharExpr>(p, value.at(0));
    }

    auto stream = std::stringstream{};
    stream << *curr_token_;
    syntactic_error("UNRECOGNIZED PRIMARY EXPRESSION: %", stream.str());
    return std::make_shared<EmptyExpr>(p);
}

auto Parser::is_assignment_operator() -> bool {
    return peek(TokenType::ASSIGN) or peek(TokenType::PLUS_ASSIGN) or peek(TokenType::MINUS_ASSIGN)
           or peek(TokenType::MULTIPLY_ASSIGN) or peek(TokenType::DIVIDE_ASSIGN);
}