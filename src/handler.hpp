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
    auto add_file(const std::string filename) -> bool;
    auto get_file_contents(const std::string& filename) -> std::shared_ptr<std::string>;

    auto report_error(std::string const& filename, std::string const& message, std::string const& token, Position pos)
        -> void;
    auto
    report_minor_error(std::string const& filename, std::string const& message, std::string const& token, Position pos)
        -> void;

    auto parse_cl_args(int argc, std::vector<std::string> argv) -> bool;
    auto tokens_mode() const noexcept -> bool {
        return tokens_;
    }

    static auto help() -> void;

    std::string source_filename = {};

    static const Type VOID_TYPE;
    static const Type I64_TYPE;
    static const Type ERROR_TYPE;
    static const Type BOOL_TYPE;
    static const Type UNKNOWN_TYPE;

 private:
    std::map<std::string, std::shared_ptr<std::string>> filename_to_contents_ = {};
    std::map<std::string, std::vector<std::string>> filename_to_lines_ = {};
    auto log_lines(const std::string& filename, int line, int col) -> void;
    std::string const ANSI_RED_ = "\033[31m";
    std::string const ANSI_RESET_ = "\033[0m";
    std::string const ANSI_YELLOW_ = "\033[33m";
    std::string const ANSI_BLUE_ = "\033[34m";
    bool quiet_ = false, run_ = false, parser_raw_ = false, tokens_ = false, parser_ = false;
    bool assembly_ = false, stats_ = false;
    std::string output_filename_ = "a.out";

    size_t num_errors_ = 0;
};

#endif // HANDLER_HPP