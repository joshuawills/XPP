#include <iostream>

#include "./handler.hpp"
#include "./lexer.hpp"
#include "./parser.hpp"

auto main(int argc, char** argv) -> int {
    if (argc != 2) {
        std::cerr << "Expected filename as argument\n";
    }

    auto filename = std::string{argv[1]};
    auto handler = std::make_shared<Handler>();

    handler->add_file(filename);

    auto lexer = Lexer(filename, handler);
    auto tokens = lexer.tokenize();

    auto parser = Parser(tokens, filename, handler);
    auto module = parser.parse();

    return 0;
}