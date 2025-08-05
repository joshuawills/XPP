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

auto Token::str() const -> std::string {
    return "Token{"
           "lexeme: '"
           + lexeme_
           + "', "
             "position: {line_start: "
           + std::to_string(position_.line_start_) + ", line_end:" + std::to_string(position_.line_end_)
           + ", col_start: " + std::to_string(position_.col_start_) + ", col_end: " + std::to_string(position_.col_end_)
           + "}, "
             "type: "
           + std::to_string(static_cast<int>(type_)) + "}";
}

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType> {
    auto const lookup_map = std::map<std::string, TokenType>{{"fn", TokenType::FN},
                                                             {"using", TokenType::USING},
                                                             {"as", TokenType::AS},
                                                             {"i64", TokenType::TYPE},
                                                             {"bool", TokenType::TYPE},
                                                             {"void", TokenType::TYPE},
                                                             {"mut", TokenType::MUT},
                                                             {"let", TokenType::LET},
                                                             {"return", TokenType::RETURN},
                                                             {"true", TokenType::TRUE},
                                                             {"false", TokenType::FALSE}};

    return lookup_map.find(str) != lookup_map.end() ? std::make_optional(lookup_map.at(str)) : std::nullopt;
}

auto token_type_to_str(TokenType t) -> std::string {
    switch (t) {
    case TokenType::IDENT: return "IDENT";
    case TokenType::FN: return "FN";
    case TokenType::LESS_THAN: return "LESS_THAN";
    case TokenType::GREATER_THAN: return "GREATER_THAN";
    case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
    case TokenType::LESS_EQUAL: return "LESS_EQUAL";
    case TokenType::OPEN_CURLY: return "OPEN_CURLY";
    case TokenType::CLOSE_CURLY: return "CLOSE_CURLY";
    case TokenType::OPEN_BRACKET: return "OPEN_BRACKET";
    case TokenType::CLOSE_BRACKET: return "CLOSE_BRACKET";
    case TokenType::COLON: return "COLON";
    case TokenType::SEMICOLON: return "SEMICOLON";
    case TokenType::TYPE: return "TYPE";
    case TokenType::AS: return "AS";
    case TokenType::USING: return "USING";
    case TokenType::INTEGER: return "INTEGER";
    case TokenType::COMMA: return "COMMA";
    case TokenType::MUT: return "MUT";
    case TokenType::LET: return "LET";
    case TokenType::ASSIGN: return "ASSIGN";
    case TokenType::LOGICAL_OR: return "LOGICAL_OR";
    case TokenType::LOGICAL_AND: return "LOGICAL_AND";
    case TokenType::EQUAL: return "EQUAL";
    case TokenType::NOT_EQUAL: return "NOT_EQUAL";
    case TokenType::NEGATE: return "NEGATE";
    case TokenType::PLUS: return "PLUS";
    case TokenType::MINUS: return "MINUS";
    case TokenType::MULTIPLY: return "MULTIPLY";
    case TokenType::DIVIDE: return "DIVIDE";
    case TokenType::TRUE: return "TRUE";
    case TokenType::FALSE: return "FALSE";
    case TokenType::RETURN: return "RETURN";
    default: return "UNKNOWN";
    }
}

auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void {
    for (const auto& token : tokens) {
        std::cout << token->str() << "\n";
    }
}