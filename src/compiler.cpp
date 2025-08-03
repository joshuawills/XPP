#include <iostream>

#include "./lexer.hpp"
#include "./parser.hpp"

auto main(int argc, char **argv) -> int {

    if (argc != 2) {
        std::cerr << "Expected filename as argument\n";
    }

    auto filename = std::string(argv[1]);

    auto lexer = Lexer(filename);
    auto tokens = lexer.tokenize();

    auto parser = Parser(tokens, filename);
    auto module = parser.parse();

    return 0;
}