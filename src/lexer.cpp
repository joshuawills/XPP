#include "./lexer.hpp"

#include <cctype>
#include <iostream>

auto isalpha_or_under(const char& c) -> bool {
    return (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z') or c == '_';
}

auto Lexer::peek(char c, int j) -> bool {
    return current_pos_ + j < contents_->size() and (*contents_)[current_pos_ + j] == c;
}

auto Lexer::tokenize() -> std::vector<std::shared_ptr<Token>> {
    auto tokens = std::vector<std::shared_ptr<Token>>{};

    contents_ = handler_->get_file_contents(filename_);
    auto const size = contents_->size();

    while (current_pos_ < size) {
        skip_whitespace_and_comments();
        auto const& t = generate_token();
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
    case '.': {
        if (peek('.', 1) and peek('.', 2)) {
            consume();
            consume();
            consume();
            return Token{"...", line_, col_ - 1, col_ - 1, TokenType::TYPE};
        }
        std::cerr << "Invalid character";
        exit(EXIT_FAILURE);
    }
    case '>': {
        consume();
        if (peek('=')) {
            consume();
            return Token{">=", line_, col_ - 2, col_ - 1, TokenType::GREATER_EQUAL};
        }
        return Token{">", line_, col_ - 1, col_ - 1, TokenType::GREATER_THAN};
    }
    case '<': {
        consume();
        if (peek('=')) {
            consume();
            return Token{"<=", line_, col_ - 2, col_ - 1, TokenType::LESS_EQUAL};
        }
        return Token{"<", line_, col_ - 1, col_ - 1, TokenType::LESS_THAN};
    }
    case '{': consume(); return Token{"{", line_, col_ - 1, col_ - 1, TokenType::OPEN_CURLY};
    case '}': consume(); return Token{"}", line_, col_ - 1, col_ - 1, TokenType::CLOSE_CURLY};
    case ':': consume(); return Token{":", line_, col_ - 1, col_ - 1, TokenType::COLON};
    case ';': consume(); return Token{";", line_, col_ - 1, col_ - 1, TokenType::SEMICOLON};
    case '(': consume(); return Token{"(", line_, col_ - 1, col_ - 1, TokenType::OPEN_BRACKET};
    case ')': consume(); return Token{")", line_, col_ - 1, col_ - 1, TokenType::CLOSE_BRACKET};
    case ',': consume(); return Token{",", line_, col_ - 1, col_ - 1, TokenType::COMMA};
    case '=': {
        consume();
        if (peek('=')) {
            consume();
            return Token{"==", line_, col_ - 1, col_ - 1, TokenType::EQUAL};
        }
        return Token{"=", line_, col_ - 1, col_ - 1, TokenType::ASSIGN};
    }
    case '!': {
        consume();
        if (peek('=')) {
            consume();
            return Token{"!=", line_, col_ - 1, col_ - 1, TokenType::NOT_EQUAL};
        }
        return Token{"!", line_, col_ - 1, col_ - 1, TokenType::NEGATE};
    }
    case '|': {
        consume();
        if (peek('|')) {
            consume();
            return Token{"||", line_, col_ - 2, col_ - 1, TokenType::LOGICAL_OR};
        }
        std::cerr << "Unexpected character '|' at line " << line_ << ", column " << col_ << "\n";
        exit(EXIT_FAILURE);
    }
    case '&': {
        consume();
        if (peek('&')) {
            consume();
            return Token{"&&", line_, col_ - 2, col_ - 1, TokenType::LOGICAL_AND};
        }
        return Token{"&", line_, col_ - 1, col_ - 1, TokenType::AMPERSAND};
    }
    case '-': consume(); return Token{"-", line_, col_ - 1, col_ - 1, TokenType::MINUS};
    case '+': consume(); return Token{"+", line_, col_ - 1, col_ - 1, TokenType::PLUS};
    case '/': consume(); return Token{"/", line_, col_ - 1, col_ - 1, TokenType::DIVIDE};
    case '*': consume(); return Token{"*", line_, col_ - 1, col_ - 1, TokenType::MULTIPLY};
    case '"': {
        consume();
        auto buf = std::string{};
        while (current_pos_ < contents_->size() and !peek('"')) {
            if (peek('\n')) {
                std::cerr << "ERROR: Currently not supporting multiline strings\n";
                exit(EXIT_FAILURE);
            }

            if (peek('\\')) {
                if (!valid_escape()) {
                    std::cerr << "ERROR: Invalid escape sequence\n";
                    exit(EXIT_FAILURE);
                }
                else {
                    consume();
                    buf += consume_escape();
                }
            }
            else {
                buf += consume();
            }
        }
        consume();
        return Token{buf, line_, col_ - buf.size(), col_ - 1, TokenType::STRING_LITERAL};
    }
    case '\'': {
        consume();
        auto buf = std::string{};
        while (current_pos_ < contents_->size() and !peek('\'')) {
            if (peek('\n')) {
                std::cerr << "ERROR: Currently not supporting multiline chars\n";
                exit(EXIT_FAILURE);
            }

            if (peek('\\')) {
                if (!valid_escape()) {
                    std::cerr << "ERROR: Invalid escape sequence\n";
                    exit(EXIT_FAILURE);
                }
                else {
                    consume();
                    buf += consume_escape();
                }
            }
            else {
                buf += consume();
            }
        }
        consume();
        return Token{buf, line_, col_ - buf.size(), col_ - 1, TokenType::CHAR_LITERAL};
    }
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
        if (buf == "else" and else_if_case()) {
            buf += consume();
            buf += consume();
            buf += consume();
        }
        auto const type = get_type_from_lexeme(buf);
        if (type.has_value()) {
            return Token{buf, line_, col_ - buf.size(), col_ - 1, *type};
        }
        else {
            return Token{buf, line_, col_ - buf.size(), col_ - 1, TokenType::IDENT};
        }
    }

    while (current_pos_ < contents_->size() and isdigit((*contents_)[current_pos_])) {
        buf += consume();
    }

    if (!buf.empty()) {
        return Token{buf, line_, col_ - buf.size(), col_ - 1, TokenType::INTEGER};
    }

    if (current_pos_ < contents_->size()) {
        std::cerr << "Unexpected character '" << (*contents_)[current_pos_] << "' at line " << line_ << ", column "
                  << col_ << "\n";
        exit(EXIT_FAILURE);
    }

    return std::nullopt;
}

auto Lexer::skip_whitespace() -> void {
    while (current_pos_ < contents_->size() and isspace(contents_->at(current_pos_))) {
        consume();
    }
}

auto Lexer::skip_whitespace_and_comments() -> void {
    skip_whitespace();
    if (peek('/') and peek('/', 1)) {
        while (current_pos_ < contents_->size() and !peek('\n')) {
            consume();
        }
        consume();
    }
    else if (peek('/') and peek('*', 1)) {
        while (!(peek('*') and peek('/', 1))) {
            consume();
        }
        consume();
        consume();
    }
    skip_whitespace();
    if (is_comment()) {
        skip_whitespace_and_comments();
    }
}

auto Lexer::is_comment() -> bool {
    return peek('/') and (peek('/', 1) or peek('*', 1));
}

auto Lexer::consume() -> char {
    auto const curr_char = contents_->at(current_pos_);
    if (curr_char == '\n') {
        ++line_;
        col_ = 1;
    }
    else if (curr_char == '\t') {
        col_ += 4;
    }
    else {
        ++col_;
    }
    ++current_pos_;
    return curr_char;
}

auto Lexer::consume_escape() -> char {
    if (peek('b')) {
        consume();
        return '\b';
    }
    else if (peek('f')) {
        consume();
        return '\f';
    }
    else if (peek('n')) {
        consume();
        return '\n';
    }
    else if (peek('r')) {
        consume();
        return '\r';
    }
    else if (peek('t')) {
        consume();
        return '\t';
    }
    else if (peek('\'')) {
        consume();
        return '\'';
    }
    else if (peek('"')) {
        consume();
        return '\"';
    }
    else if (peek('\\')) {
        consume();
        return '\\';
    }
    else if (peek('0')) {
        consume();
        return '\0';
    }
    std::cerr << "UNREACHABLE Lexer::consume_escape\n";
    return 'a';
}

auto Lexer::valid_escape() -> bool {
    return peek('b') or peek('f') or peek('n') or peek('r') or peek('t') or peek('\'') or peek('"') or peek('\\')
           or peek('0');
}

auto Lexer::else_if_case() -> bool {
    return peek(' ') and peek('i', 1) and peek('f', 2);
}