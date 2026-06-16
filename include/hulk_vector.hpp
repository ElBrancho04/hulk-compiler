#ifndef HULK_VECTOR_HPP
#define HULK_VECTOR_HPP

#include <stdexcept>
#include <vector>

#include "value.hpp"

class HulkVector {
public:
    std::vector<Value> elements;

    HulkVector() : iter_index_(0), iter_started_(false) {}

    explicit HulkVector(std::vector<Value> elements)
        : elements(std::move(elements)), iter_index_(0), iter_started_(false) {}

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

    Value next() {
        if (!iter_started_) {
            iter_started_ = true;
            iter_index_ = 0;
        } else {
            ++iter_index_;
        }
        return Value::Boolean(iter_index_ < elements.size());
    }

    Value current() const {
        if (!iter_started_ || iter_index_ >= elements.size()) {
            return Value::Null();
        }
        return elements[iter_index_];
    }

private:
    std::size_t iter_index_;
    bool iter_started_;
};

#endif
