#include <iostream>

#include "./lexer.hpp"

auto main(int argc, char **argv) -> int {

    if (argc != 2) {
        std::cerr << "Expected filename as argument\n";
    }
    
    auto lexer = Lexer(std::string(argv[1]));
    auto tokens = lexer.tokenize();
    log_tokens(tokens);

    return 0;
}