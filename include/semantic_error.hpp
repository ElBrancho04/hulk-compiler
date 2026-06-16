#ifndef SEMANTIC_ERROR_HPP
#define SEMANTIC_ERROR_HPP

#include <stdexcept>
#include <string>

class SemanticError : public std::runtime_error {
public:
    SemanticError(int line, int col, const std::string& message)
        : std::runtime_error(format_message(line, col, message)), line_(line), col_(col) {}

    // Backward-compatible constructor (col defaults to 0)
    SemanticError(int line, const std::string& message)
        : std::runtime_error(format_message(line, 0, message)), line_(line), col_(0) {}

    int line() const { return line_; }
    int col() const { return col_; }

private:
    int line_;
    int col_;

    static std::string format_message(int line, int col, const std::string& message) {
        return "(" + std::to_string(line) + "," + std::to_string(col) + ") SEMANTIC: " + message;
    }
};

#endif