#ifndef LEXER_HPP
#define LEXER_HPP

#include "./handler.hpp"
#include "./token.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Lexer {
 public:
    Lexer(const std::string filename, const std::shared_ptr<Handler>& handler)
    : filename_{filename}
    , handler_{handler} {}

    ~Lexer() = default;

    auto tokenize() -> std::vector<std::shared_ptr<Token>>;

 private:
    auto skip_whitespace() -> void;
    auto skip_whitespace_and_comments() -> void;
    auto generate_token() -> std::optional<Token>;
    auto consume() -> char;
    auto is_comment() -> bool;
    auto peek(char c, int j = 0) -> bool;

    const std::string filename_;
    std::shared_ptr<Handler> handler_;
    std::shared_ptr<std::string> contents_;
    size_t current_pos_ = 0, current_line_ = 1, current_col_ = 1;
};

#endif // LEXER_HPP