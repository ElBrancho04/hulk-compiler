#ifndef HULK_OBJECT_HPP
#define HULK_OBJECT_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "value.hpp"

class HulkObject {
public:
    std::string type_name;
    std::vector<std::string> ancestors;
    std::unordered_map<std::string, Value> attributes;

    virtual ~HulkObject() = default;

    explicit HulkObject(std::string type_name, std::vector<std::string> ancestors = {})
        : type_name(std::move(type_name)), ancestors(std::move(ancestors)) {}

    bool hasAttribute(const std::string& name) const {
        return attributes.find(name) != attributes.end();
    }

    Value getAttribute(const std::string& name) const {
        auto it = attributes.find(name);
        if (it == attributes.end()) {
            return Value::Null();
        }
        return it->second;
    }

    void setAttribute(const std::string& name, const Value& value) {
        attributes[name] = value;
    }
};

#endif
