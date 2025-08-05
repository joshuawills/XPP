#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./token.hpp"
#include "./type.hpp"

class Handler {
 public:
    Handler() = default;
    Handler(bool is_quiet)
    : quiet_{is_quiet} {}

    auto add_file(std::string const filename) -> bool;
    auto get_file_contents(std::string const& filename) -> std::shared_ptr<std::string>;

    auto
    report_error(std::string const& filename, std::string const& message, std::string const& token, Position const& pos)
        -> void;
    auto report_minor_error(std::string const& filename,
                            std::string const& message,
                            std::string const& token,
                            Position const& pos) -> void;

    auto parse_cl_args(int argc, std::vector<std::string> const& argv) -> bool;

    auto tokens_mode() const noexcept -> bool {
        return tokens_;
    }

    auto parser_mode() const noexcept -> bool {
        return parser_;
    }

    auto llvm_mode() const noexcept -> bool {
        return llvm_ir_;
    }

    auto get_output_filename() -> std::string& {
        return output_filename_;
    }
    auto get_object_filename() -> std::string& {
        return object_filename_;
    }

    auto get_assembly_filename() -> std::string& {
        return assembly_filename_;
    }

    auto get_llvm_filename() -> std::string& {
        return llvm_filename_;
    }

    auto is_assembly() const noexcept -> bool {
        return assembly_;
    }

    static auto help() -> void;

    std::string source_filename = {};

    static const Type VOID_TYPE;
    static const Type I64_TYPE;
    static const Type ERROR_TYPE;
    static const Type BOOL_TYPE;
    static const Type UNKNOWN_TYPE;

    size_t num_errors_ = 0;

 private:
    std::map<std::string, std::shared_ptr<std::string>> filename_to_contents_ = {};
    std::map<std::string, std::vector<std::string>> filename_to_lines_ = {};
    auto log_lines(const std::string& filename, int line, int col) -> void;
    std::string const ANSI_RED_ = "\033[31m";
    std::string const ANSI_RESET_ = "\033[0m";
    std::string const ANSI_YELLOW_ = "\033[33m";
    std::string const ANSI_BLUE_ = "\033[34m";
    bool quiet_ = false, run_ = false, tokens_ = false, parser_ = false;
    bool assembly_ = false, stats_ = false, llvm_ir_ = false;
    std::string output_filename_ = "a.out";
    std::string object_filename_ = "default.o";
    std::string assembly_filename_ = "default.s";
    std::string llvm_filename_ = "default.ll";
};

#endif // HANDLER_HPP