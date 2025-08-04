#include "./lexer.hpp"

#include <cctype>
#include <iostream>

auto isalpha_or_under(const char& c) -> bool {
    return (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z') or c == '_';
}

auto Lexer::is_curr_char(char c) -> bool {
    return current_pos_ < contents_->size() and (*contents_)[current_pos_] == c;
}

auto Lexer::tokenize() -> std::vector<std::shared_ptr<Token>> {
    std::vector<std::shared_ptr<Token>> tokens;

    contents_ = handler_->get_file_contents(filename_);
    auto const size = contents_->size();

    while (current_pos_ < size) {
        skip_whitespace();
        auto t = generate_token();
        if (t.has_value()) {
            tokens.emplace_back(std::make_shared<Token>(*t));
        }
        else {
            break;
        }
    }

    return tokens;
}

auto Lexer::generate_token() -> std::optional<Token> {
    if (current_pos_ >= contents_->size()) {
        return std::nullopt;
    }

    switch ((*contents_)[current_pos_]) {
    case '>': {
        consume();
        if (is_curr_char('=')) {
            consume();
            return Token{">=", current_line_, current_col_ - 2, current_col_ - 1, TokenType::GREATER_EQUAL};
        }
        return Token{">", current_line_, current_col_ - 1, current_col_ - 1, TokenType::GREATER_THAN};
    }
    case '<': {
        consume();
        if (is_curr_char('=')) {
            consume();
            return Token{"<=", current_line_, current_col_ - 2, current_col_ - 1, TokenType::LESS_EQUAL};
        }
        return Token{"<", current_line_, current_col_ - 1, current_col_ - 1, TokenType::LESS_THAN};
    }
    case '{': consume(); return Token{"{", current_line_, current_col_ - 1, current_col_ - 1, TokenType::OPEN_CURLY};
    case '}': consume(); return Token{"}", current_line_, current_col_ - 1, current_col_ - 1, TokenType::CLOSE_CURLY};
    case ':': consume(); return Token{":", current_line_, current_col_ - 1, current_col_ - 1, TokenType::COLON};
    case ';': consume(); return Token{";", current_line_, current_col_ - 1, current_col_ - 1, TokenType::SEMICOLON};
    case '(': consume(); return Token{"(", current_line_, current_col_ - 1, current_col_ - 1, TokenType::OPEN_BRACKET};
    case ')': consume(); return Token{")", current_line_, current_col_ - 1, current_col_ - 1, TokenType::CLOSE_BRACKET};
    case ',': consume(); return Token{",", current_line_, current_col_ - 1, current_col_ - 1, TokenType::COMMA};
    case '=': {
        consume();
        if (is_curr_char('=')) {
            consume();
            return Token{"==", current_line_, current_col_ - 1, current_col_ - 1, TokenType::EQUAL};
        }
        return Token{"=", current_line_, current_col_ - 1, current_col_ - 1, TokenType::ASSIGN};
    }
    case '!': {
        consume();
        if (is_curr_char('=')) {
            consume();
            return Token{"!=", current_line_, current_col_ - 1, current_col_ - 1, TokenType::NOT_EQUAL};
        }
        return Token{"!", current_line_, current_col_ - 1, current_col_ - 1, TokenType::NEGATE};
    }
    case '|': {
        consume();
        if (is_curr_char('|')) {
            consume();
            return Token{"||", current_line_, current_col_ - 2, current_col_ - 1, TokenType::LOGICAL_OR};
        }
        std::cerr << "Unexpected character '|' at line " << current_line_ << ", column " << current_col_ << "\n";
        exit(EXIT_FAILURE);
    }
    case '&': {
        consume();
        if (is_curr_char('&')) {
            consume();
            return Token{"&&", current_line_, current_col_ - 2, current_col_ - 1, TokenType::LOGICAL_AND};
        }
        std::cerr << "Unexpected character '&' at line " << current_line_ << ", column " << current_col_ << "\n";
        exit(EXIT_FAILURE);
    }
    case '-': consume(); return Token{"-", current_line_, current_col_ - 1, current_col_ - 1, TokenType::MINUS};
    case '+': consume(); return Token{"+", current_line_, current_col_ - 1, current_col_ - 1, TokenType::PLUS};
    case '/': consume(); return Token{"/", current_line_, current_col_ - 1, current_col_ - 1, TokenType::DIVIDE};
    case '*': consume(); return Token{"*", current_line_, current_col_ - 1, current_col_ - 1, TokenType::MULTIPLY};
    }

    auto buf = std::string{};
    if (current_pos_ < contents_->size() and isalpha_or_under((*contents_)[current_pos_])) {
        while (current_pos_ < contents_->size()
               and (isalpha_or_under((*contents_)[current_pos_]) or isdigit((*contents_)[current_pos_])))
        {
            buf += consume();
        }
    }

    if (!buf.empty()) {
        auto const type = get_type_from_lexeme(buf);
        if (type.has_value()) {
            return Token{buf, current_line_, current_col_ - buf.size(), current_col_ - 1, *type};
        }
        else {
            return Token{buf, current_line_, current_col_ - buf.size(), current_col_ - 1, TokenType::IDENT};
        }
    }

    while (current_pos_ < contents_->size() and isdigit((*contents_)[current_pos_])) {
        buf += consume();
    }

    if (!buf.empty()) {
        return Token{buf, current_line_, current_col_ - buf.size(), current_col_ - 1, TokenType::INTEGER};
    }

    return std::nullopt;
}

auto Lexer::skip_whitespace() -> void {
    while (current_pos_ < contents_->size() and isspace(contents_->at(current_pos_))) {
        consume();
    }
}

auto Lexer::consume() -> char {
    auto const curr_char = contents_->at(current_pos_);
    if (curr_char == '\n') {
        ++current_line_;
        current_col_ = 1;
    }
    else if (curr_char == '\t') {
        current_col_ += 4;
    }
    else {
        ++current_col_;
    }
    ++current_pos_;
    return curr_char;
}