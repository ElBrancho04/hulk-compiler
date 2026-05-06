#ifndef SEMANTIC_ERROR_HPP
#define SEMANTIC_ERROR_HPP

#include <stdexcept>
#include <string>

class SemanticError : public std::runtime_error {
public:
    SemanticError(int line, const std::string& message)
        : std::runtime_error(format_message(line, message)), line_(line) {}

    int line() const { return line_; }

private:
    int line_;

    static std::string format_message(int line, const std::string& message) {
        return "[Semantic Error] line " + std::to_string(line) + ": " + message;
    }
};

#endif
