#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <cstddef>
#include <optional>
#include <vector>
#include <memory>

struct Position {
    size_t line_num_, col_start_, col_end_;
};

enum class TokenType {
    IDENT,
    FN,
    LESS_THAN,
    GREATER_THAN,
    OPEN_CURLY,
    CLOSE_CURLY,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    COLON,
    SEMICOLON,
    TYPE,
    AS,
    USING,
    INTEGER
};


class Token {
    public:
        Token() = default;
        Token(std::string lexeme, size_t line_num, size_t col_start, size_t col_end, TokenType type);
        ~Token() = default;
        auto str() const -> std::string;
    private:
        std::string lexeme_;
        Position position_;
        TokenType type_;
};

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType>;
auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void;

#endif // TOKEN_HPP