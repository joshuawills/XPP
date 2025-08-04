#include <iostream>

#include "./handler.hpp"
#include "./lexer.hpp"
#include "./parser.hpp"

auto main(int argc, char** argv) -> int {
    auto handler = std::make_shared<Handler>();

    if (!handler->parse_cl_args(argc, std::vector<std::string>(argv, argv + argc))) {
        return EXIT_FAILURE;
    }

    handler->add_file(handler->source_filename);

    auto lexer = Lexer(handler->source_filename, handler);
    auto tokens = lexer.tokenize();

    auto parser = Parser(tokens, handler->source_filename, handler);
    auto module = parser.parse();

    return 0;
}