#include "./token.hpp"

#include <map>
#include <iostream>

Token::Token(std::string lexeme, size_t line_num, size_t col_start, size_t col_end, TokenType type)
    : lexeme_{std::move(lexeme)}, position_{line_num, col_start, col_end}, type_{type} {}

auto Token::str() const -> std::string {
    return "Token{"
           "lexeme: '" + lexeme_ + "', "
           "position: {line: " + std::to_string(position_.line_num_) +
           ", col_start: " + std::to_string(position_.col_start_) +
           ", col_end: " + std::to_string(position_.col_end_) + "}, "
           "type: " + std::to_string(static_cast<int>(type_)) + "}";
}

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType> {
    auto const lookup_map = std::map<std::string, TokenType>{
        {"fn", TokenType::FN},
        {"using", TokenType::USING},
        {"as", TokenType::AS},
        {"i64", TokenType::TYPE},
        {"void", TokenType::TYPE},
        {"mut", TokenType::MUT},
        {"let", TokenType::LET}
    };

    return lookup_map.find(str) != lookup_map.end() ? std::make_optional(lookup_map.at(str)) : std::nullopt;
}

auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void {
    for (const auto& token : tokens) {
        std::cout << token->str() << "\n";
    }
}