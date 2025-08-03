#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "./token.hpp"

class Lexer {
    public:
        Lexer(const std::string filename):
            filename_{filename} {}

        ~Lexer() = default;

        auto tokenize() -> std::vector<std::shared_ptr<Token>>;
    private:
        auto skip_whitespace() -> void;
        auto generate_token() -> std::optional<Token>;
        auto consume() -> char;
        auto is_curr_char(char c) -> bool;

        const std::string filename_;
        std::string contents_;
        size_t current_pos_ = 0, current_line_ = 1, current_col_ = 1;
};

#endif // LEXER_HPP