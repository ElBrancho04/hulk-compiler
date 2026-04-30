#ifndef HULK_VECTOR_HPP
#define HULK_VECTOR_HPP

#include <stdexcept>
#include <vector>

#include "value.hpp"

class HulkVector {
public:
    std::vector<Value> elements;

    HulkVector() = default;

    explicit HulkVector(std::vector<Value> elements)
        : elements(std::move(elements)) {}

    std::size_t size() const {
        return elements.size();
    }

    Value& at(std::size_t index) {
        if (index >= elements.size()) {
            throw std::out_of_range("HulkVector index out of range");
        }
        return elements[index];
    }

    const Value& at(std::size_t index) const {
        if (index >= elements.size()) {
            throw std::out_of_range("HulkVector index out of range");
        }
        return elements[index];
    }
};

#endif
