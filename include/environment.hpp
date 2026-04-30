#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "runtime_error.hpp"
#include "value.hpp"

class Environment : public std::enable_shared_from_this<Environment> {
public:
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> parent;

    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> parent)
        : parent(std::move(parent)) {}

    void define(const std::string& name, const Value& value) {
        values[name] = value;
    }

    Value get(const std::string& name) const {
        auto it = values.find(name);
        if (it != values.end()) {
            return it->second;
        }
        if (parent) {
            return parent->get(name);
        }
        throw RuntimeError("Undefined variable: " + name);
    }

    void assign(const std::string& name, const Value& value) {
        auto it = values.find(name);
        if (it != values.end()) {
            it->second = value;
            return;
        }
        if (parent) {
            parent->assign(name, value);
            return;
        }
        throw RuntimeError("Undefined variable: " + name);
    }
};

#endif
