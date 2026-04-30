#ifndef RUNTIME_ERROR_HPP
#define RUNTIME_ERROR_HPP

#include <stdexcept>
#include <string>

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& message)
        : std::runtime_error(message) {}
};

#endif
