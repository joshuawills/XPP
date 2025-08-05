#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Position {
    size_t line_start_, line_end_, col_start_, col_end_;
};

auto operator<<(std::ostream& os, Position const& p) -> std::ostream&;

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
    TRUE,
    FALSE,
    RETURN,
    EXTERN,
    STRING_LITERAL,
    CHAR_LITERAL,
};

class Token {
 public:
    Token() = default;
    Token(std::string lexeme, size_t line_num, size_t line_end, size_t col_start, size_t col_end, TokenType type);
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

    auto type_matches(TokenType other) -> bool {
        return type_ == other;
    }

 private:
    std::string lexeme_;
    Position position_;
    TokenType type_;
};

auto get_type_from_lexeme(std::string const& str) -> std::optional<TokenType>;
auto log_tokens(const std::vector<std::shared_ptr<Token>>& tokens) -> void;
auto token_type_to_str(TokenType t) -> std::string;

#endif // TOKEN_HPP