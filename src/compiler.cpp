#include <iostream>
#include <sstream>

#include "./emitter.hpp"
#include "./handler.hpp"
#include "./lexer.hpp"
#include "./parser.hpp"
#include "./verifier.hpp"

auto main(int argc, char** argv) -> int {
    auto handler = std::make_shared<Handler>();

    if (!handler->parse_cl_args(argc, std::vector<std::string>(argv, argv + argc))) {
        return EXIT_FAILURE;
    }

    handler->add_file(handler->source_filename);

    auto lexer = Lexer(handler->source_filename, handler);
    auto tokens = lexer.tokenize();

    if (handler->tokens_mode()) {
        log_tokens(tokens);
        exit(EXIT_SUCCESS);
    }

    auto parser = Parser(tokens, handler->source_filename, handler);
    auto module = parser.parse();

    if (handler->num_errors_) {
        exit(EXIT_FAILURE);
    }

    if (handler->parser_mode()) {
        std::cout << *module;
        exit(EXIT_SUCCESS);
    }

    auto modules = std::make_shared<AllModules>();
    modules->add_main_module(module);

    auto verifier = std::make_shared<Verifier>(handler, modules);
    verifier->check(handler->source_filename, true);

    if (handler->num_errors_) {
        exit(EXIT_FAILURE);
    }

    auto emitter = std::make_shared<Emitter>(modules, module, handler);
    emitter->emit();

    if (handler->run_exe()) {
        auto stream = std::string{"./"};
        stream += handler->get_output_filename();
        system(stream.c_str());
    }

    return 0;
}