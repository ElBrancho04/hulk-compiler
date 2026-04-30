#ifndef HULK_RANGE_HPP
#define HULK_RANGE_HPP

#include <string>

#include "hulk_object.hpp"
#include "value.hpp"

class HulkRange : public HulkObject {
public:
    double start;
    double end;
    double current_value;
    bool started;

    HulkRange(double start, double end)
        : HulkObject("Range"), start(start), end(end), current_value(start), started(false) {}

    Value current() const {
        if (!started) {
            return Value::Null();
        }
        return Value::Number(current_value);
    }

    Value next() {
        if (!started) {
            started = true;
            current_value = start;
        } else {
            current_value += 1.0;
        }

        if (current_value >= end) {
            return Value::Boolean(false);
        }

        return Value::Boolean(true);
    }
};

#endif
