#include "./handler.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <streambuf>

const Type Handler::VOID_TYPE = Type{TypeSpec::VOID};
const Type Handler::I64_TYPE = Type{TypeSpec::I64};
const Type Handler::ERROR_TYPE = Type{TypeSpec::ERROR};
const Type Handler::BOOL_TYPE = Type{TypeSpec::BOOL};
const Type Handler::UNKNOWN_TYPE = Type{TypeSpec::UNKNOWN};
const Type Handler::CHAR_POINTER_TYPE = Type{TypeSpec::POINTER, std::nullopt, std::make_shared<Type>(TypeSpec::I8)};
const Type Handler::VARIATIC_TYPE = Type{TypeSpec::VARIATIC};

auto read_file(std::string const& filename) -> std::optional<std::string> {
    auto stream = std::ifstream{filename};
    if (!stream) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

auto Handler::add_file(std::string const filename) -> bool {
    if (filename_to_contents_.find(filename) == filename_to_contents_.end()) {
        auto const& contents = read_file(filename);
        if (!contents) {
            std::cerr << "Failed to read file: " << filename << "\n";
            return false;
        }
        filename_to_contents_[filename] = std::make_shared<std::string>(*contents);
        filename_to_lines_[filename] = std::vector<std::string>{};

        std::istringstream stream(*contents);
        std::string line;
        while (std::getline(stream, line)) {
            filename_to_lines_[filename].push_back(line);
        }
        return true;
    }
    return false;
}

auto Handler::get_file_contents(const std::string& filename) -> std::shared_ptr<std::string> {
    auto const& it = filename_to_contents_.find(filename);
    if (it != filename_to_contents_.end()) {
        return it->second;
    }
    return nullptr;
}

auto Handler::report_error(std::string const& filename,
                           std::string const& message,
                           std::string const& token,
                           Position const& pos) -> void {
    std::cout << ANSI_RED_ << "ERROR: " << ANSI_RESET_;
    for (auto c = 0u; c < message.size(); ++c) {
        if (message.at(c) == '%') {
            std::cout << token;
        }
        else {
            std::cout << message.at(c);
        }
    }
    std::cout << "\n";
    log_lines(filename, pos.line_start_, pos.col_start_);
    ++num_errors_;
}

auto Handler::report_minor_error(std::string const& filename,
                                 std::string const& message,
                                 std::string const& token,
                                 Position const& pos) -> void {
    if (quiet_)
        return;
    std::cout << ANSI_BLUE_ << "MINOR ERROR: " << ANSI_RESET_;
    for (auto c = 0u; c < message.size(); ++c) {
        if (message.at(c) == '%') {
            std::cout << token;
        }
        else {
            std::cout << message.at(c);
        }
    }
    std::cout << "\n";
    log_lines(filename, pos.line_start_, pos.col_start_);
}

auto Handler::log_lines(const std::string& filename, int line, int col) -> void {
    std::cout << ANSI_YELLOW_ << filename << ":" << line << ":" << col << ANSI_RESET_ << ":\n";
    auto const& lines = filename_to_lines_[filename];
    for (int i = line - 2; i <= line + 2; ++i) {
        if (i >= 1 and i <= (int)lines.size()) {
            std::cout << std::setw(5) << i << " | ";
            std::cout << lines[i - 1] << "\n";
        }
    }
}

auto Handler::help() -> void {
    std::cout << "X++ Compiler Options:\n";
    std::cout << "\t-h  | --help        => Provides summary of CL arguments and use of program\n";
    std::cout << "\t-r  | --run         => Will run the program after compilation\n";
    std::cout << "\t-o  | --out         => Specify the name of the executable (default to a.out)\n";
    std::cout << "\t-t  | --tokens      => Logs to stdout a summary of all the tokens\n";
    std::cout << "\t-p  | --parser      => Generates a printed parse tree\n";
    std::cout << "\t-a  | --assembly    => Generates a .s file instead of an executable\n";
    std::cout << "\t-ir | --llvm-ir     => Generates a .ll file instead of an executable\n";
    std::cout << "\t-q  | --quiet       => Silence any non-crucial warnings\n";
    std::cout << "\t-s  | --stat        => Log statistics about the compilation times\n";
    std::cout << "\nDeveloped by Joshua Wills 2025\n";
}

auto Handler::parse_cl_args(int argc, std::vector<std::string> const& argv) -> bool {
    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " [options] <file.xpp>\n";
        Handler::help();
        return false;
    }

    auto const exists_in_args = [&argv](const std::string& arg) {
        return std::find(argv.begin(), argv.end(), arg) != argv.end();
    };

    if (exists_in_args("-h") or exists_in_args("--help")) {
        Handler::help();
        return false;
    }

    run_ = exists_in_args("-r") or exists_in_args("--run");
    tokens_ = exists_in_args("-t") or exists_in_args("--tokens");
    parser_ = exists_in_args("-p") or exists_in_args("--parser");
    assembly_ = exists_in_args("-a") or exists_in_args("--assembly");
    quiet_ = exists_in_args("-q") or exists_in_args("--quiet");
    stats_ = exists_in_args("-s") or exists_in_args("--stat");
    llvm_ir_ = exists_in_args("-ir") or exists_in_args("--llvm-ir");

    if (exists_in_args("-o") or exists_in_args("--out")) {
        auto it = std::find(argv.begin(), argv.end(), "-o");
        if (it == argv.end()) {
            it = std::find(argv.begin(), argv.end(), "--out");
        }
        if (it != argv.end() and ++it != argv.end()) {
            if (assembly_) {
                assembly_filename_ = *it;
            }
            else if (llvm_ir_) {
                llvm_filename_ = *it;
            }
            else {
                output_filename_ = *it;
            }
        }
        else {
            std::cerr << "Error: No output filename specified after -o or --out\n";
            return false;
        }
    }

    std::vector<std::string> valid_cl_args = {"-h",
                                              "--help",
                                              "-r",
                                              "--run",
                                              "-o",
                                              "--out",
                                              "-t",
                                              "--tokens",
                                              "-p",
                                              "--parser",
                                              "-a",
                                              "--assembly",
                                              "-q",
                                              "--quiet",
                                              "-s",
                                              "--stat",
                                              "-ir",
                                              "--llvm-ir"};

    source_filename = argv.back();
    if (std::find(valid_cl_args.begin(), valid_cl_args.end(), source_filename) != valid_cl_args.end()) {
        std::cerr << "Error: no source file specified\n";
        return false;
    }
    else if (source_filename == output_filename_) {
        std::cerr << "Error: no source file specified\n";
        return false;
    }

    return true;
}