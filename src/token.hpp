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
    GREATER_EQUAL,
    LESS_EQUAL,
    OPEN_CURLY,
    CLOSE_CURLY,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    COLON,
    SEMICOLON,
    TYPE,
    AS,
    USING,
    INTEGER,
    COMMA,
    MUT,
    LET,
    ASSIGN,
    LOGICAL_OR,
    LOGICAL_AND,
    EQUAL,
    NOT_EQUAL,
    NEGATE,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
};


class Token {
    public:
        Token() = default;
        Token(std::string lexeme, size_t line_num, size_t col_start, size_t col_end, TokenType type);
        ~Token() = default;
        auto str() const -> std::string;
        auto pos() const -> Position {
            return position_;
        }
        auto type() const -> TokenType {
            return type_;
        }
        auto lexeme() const -> std::string {
            return lexeme_;
        }

        auto type_matches(TokenType other) -> bool { return type_ == other; }
    private:
        std::string lexeme_;
        Position position_;
        TokenType type_;
};

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType>;
auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void;

#endif // TOKEN_HPP