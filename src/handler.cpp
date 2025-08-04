#include "./handler.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <streambuf>

auto read_file(std::string const& filename) -> std::optional<std::string> {
    auto stream = std::ifstream{filename};
    if (!stream) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

auto Handler::add_file(const std::string filename) -> bool {
    if (filename_to_contents_.find(filename) == filename_to_contents_.end()) {
        auto contents = read_file(filename);
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
    auto it = filename_to_contents_.find(filename);
    if (it != filename_to_contents_.end()) {
        return it->second;
    }
    return nullptr;
}

auto Handler::report_error(std::string const& filename, std::string const& message, std::string const& token, Position pos)
    -> void {
    std::cout << ANSI_RED_ << "ERROR: " << ANSI_RESET_;
    for (size_t c = 0; c < message.size(); ++c) {
        if (message.at(c) == '%') {
            std::cout << token;
        }
        else {
            std::cout << message.at(c);
        }
    }
    std::cout << "\n";
    log_lines(filename, pos.line_num_, pos.col_start_);
    ++num_errors_;
}

auto Handler::report_minor_error(std::string const& filename,
                                 std::string const& message,
                                 std::string const& token,
                                 Position pos) -> void {
    if (is_quiet_)
        return;
    std::cout << ANSI_BLUE_ << "MINOR ERROR: " << ANSI_RESET_;
    for (size_t c = 0; c < message.size(); ++c) {
        if (message.at(c) == '%') {
            std::cout << token;
        }
        else {
            std::cout << message.at(c);
        }
    }
    std::cout << "\n";
    log_lines(filename, pos.line_num_, pos.col_start_);
}

auto Handler::log_lines(const std::string& filename, size_t line, size_t col) -> void {
    std::cout << ANSI_YELLOW_ << filename << ":" << line << ":" << col << ANSI_RESET_ << ":\n";
    auto const& lines = filename_to_lines_[filename];
    for (size_t i = line - 2; i <= line + 2; ++i) {
        if (i >= 1 and i <= lines.size()) {
            std::cout << std::setw(5) << i << " | ";
            std::cout << lines[i - 1] << "\n";
        }
    }
}