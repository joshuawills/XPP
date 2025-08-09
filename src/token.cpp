#include "./token.hpp"

#include <iostream>
#include <map>

auto operator<<(std::ostream& os, Position const& p) -> std::ostream& {
    os << "{(";
    os << p.line_start_ << ", " << p.col_start_ << ") -> (";
    os << p.line_end_ << ", " << p.col_end_ << ")}";
    return os;
}

Token::Token(std::string lexeme, size_t line_num, size_t line_end, size_t col_start, size_t col_end, TokenType type)
: lexeme_{std::move(lexeme)}
, position_{line_num, line_end, col_start, col_end}
, type_{type} {}

Token::Token(std::string lexeme, size_t line_num, size_t col_start, size_t col_end, TokenType type)
: lexeme_{std::move(lexeme)}
, position_{line_num, line_num, col_start, col_end}
, type_{type} {}

auto operator<<(std::ostream& os, Token const& t) -> std::ostream& {
    os << "Token{lexeme: '" << t.lexeme() << "', position: " << t.pos() << ", type: " << t.type() << "}";
    return os;
}

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType> {
    auto const lookup_map = std::map<std::string, TokenType>{
        {"fn", TokenType::FN},         {"using", TokenType::USING},     {"as", TokenType::AS},
        {"i64", TokenType::TYPE},      {"i32", TokenType::TYPE},        {"i8", TokenType::TYPE},
        {"u64", TokenType::TYPE},      {"u32", TokenType::TYPE},        {"u8", TokenType::TYPE},
        {"f64", TokenType::TYPE},      {"f32", TokenType::TYPE},        {"if", TokenType::IF},
        {"else", TokenType::ELSE},     {"else if", TokenType::ELSE_IF}, {"bool", TokenType::TYPE},
        {"void", TokenType::TYPE},     {"mut", TokenType::MUT},         {"let", TokenType::LET},
        {"return", TokenType::RETURN}, {"extern", TokenType::EXTERN},   {"while", TokenType::WHILE},
        {"true", TokenType::TRUE},     {"false", TokenType::FALSE}};

    return lookup_map.find(str) != lookup_map.end() ? std::make_optional(lookup_map.at(str)) : std::nullopt;
}

auto operator<<(std::ostream& os, TokenType const& t) -> std::ostream& {
    switch (t) {
    case TokenType::IDENT: os << "IDENT"; break;
    case TokenType::FN: os << "FN"; break;
    case TokenType::LESS_THAN: os << "LESS_THAN"; break;
    case TokenType::GREATER_THAN: os << "GREATER_THAN"; break;
    case TokenType::GREATER_EQUAL: os << "GREATER_EQUAL"; break;
    case TokenType::LESS_EQUAL: os << "LESS_EQUAL"; break;
    case TokenType::OPEN_CURLY: os << "OPEN_CURLY"; break;
    case TokenType::CLOSE_CURLY: os << "CLOSE_CURLY"; break;
    case TokenType::OPEN_BRACKET: os << "OPEN_BRACKET"; break;
    case TokenType::CLOSE_BRACKET: os << "CLOSE_BRACKET"; break;
    case TokenType::COLON: os << "COLON"; break;
    case TokenType::SEMICOLON: os << "SEMICOLON"; break;
    case TokenType::TYPE: os << "TYPE"; break;
    case TokenType::AS: os << "AS"; break;
    case TokenType::USING: os << "USING"; break;
    case TokenType::INTEGER: os << "INTEGER"; break;
    case TokenType::COMMA: os << "COMMA"; break;
    case TokenType::MUT: os << "MUT"; break;
    case TokenType::LET: os << "LET"; break;
    case TokenType::ASSIGN: os << "ASSIGN"; break;
    case TokenType::LOGICAL_OR: os << "LOGICAL_OR"; break;
    case TokenType::LOGICAL_AND: os << "LOGICAL_AND"; break;
    case TokenType::EQUAL: os << "EQUAL"; break;
    case TokenType::NOT_EQUAL: os << "NOT_EQUAL"; break;
    case TokenType::NEGATE: os << "NEGATE"; break;
    case TokenType::PLUS: os << "PLUS"; break;
    case TokenType::MINUS: os << "MINUS"; break;
    case TokenType::MULTIPLY: os << "MULTIPLY"; break;
    case TokenType::DIVIDE: os << "DIVIDE"; break;
    case TokenType::TRUE: os << "TRUE"; break;
    case TokenType::FALSE: os << "FALSE"; break;
    case TokenType::RETURN: os << "RETURN"; break;
    case TokenType::EXTERN: os << "EXTERN"; break;
    case TokenType::STRING_LITERAL: os << "STRING_LITERAL"; break;
    case TokenType::CHAR_LITERAL: os << "CHAR_LITERAL"; break;
    case TokenType::WHILE: os << "WHILE"; break;
    case TokenType::IF: os << "IF"; break;
    case TokenType::ELSE: os << "ELSE"; break;
    case TokenType::ELSE_IF: os << "ELSE_IF"; break;
    case TokenType::AMPERSAND: os << "AMPERSAND"; break;
    case TokenType::UNSIGNED_INTEGER: os << "UNSIGNED_INTEGER"; break;
    case TokenType::FLOAT_LITERAL: os << "FLOAT_LITERAL"; break;
    case TokenType::PLUS_PLUS: os << "PLUS_PLUS"; break;
    case TokenType::MINUS_MINUS: os << "MINUS_MINUS"; break;
    default: os << "UNKNOWN";
    }
    return os;
}

auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void {
    for (const auto& token : tokens) {
        std::cout << *token << "\n";
    }
}