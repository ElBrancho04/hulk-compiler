#ifndef VALUE_HPP
#define VALUE_HPP

#include <memory>
#include <string>

class HulkObject;
class HulkVector;

enum class ValueType {
    Number,
    String,
    Boolean,
    Object,
    Vector,
    Null
};

struct Value {
    ValueType type = ValueType::Null;
    double number_value = 0.0;
    std::string string_value;
    bool bool_value = false;
    std::shared_ptr<HulkObject> object_value;
    std::shared_ptr<HulkVector> vector_value;

    static Value Number(double value) {
        Value v;
        v.type = ValueType::Number;
        v.number_value = value;
        return v;
    }

    static Value String(std::string value) {
        Value v;
        v.type = ValueType::String;
        v.string_value = std::move(value);
        return v;
    }

    static Value Boolean(bool value) {
        Value v;
        v.type = ValueType::Boolean;
        v.bool_value = value;
        return v;
    }

    static Value Object(std::shared_ptr<HulkObject> value) {
        Value v;
        v.type = ValueType::Object;
        v.object_value = std::move(value);
        return v;
    }

    static Value Vector(std::shared_ptr<HulkVector> value) {
        Value v;
        v.type = ValueType::Vector;
        v.vector_value = std::move(value);
        return v;
    }

    static Value Null() {
        return Value{};
    }

    bool sameType(const Value& other) const {
        return type == other.type;
    }
};

inline bool operator==(const Value& left, const Value& right) {
    if (left.type != right.type) {
        return false;
    }

    switch (left.type) {
        case ValueType::Number:
            return left.number_value == right.number_value;
        case ValueType::String:
            return left.string_value == right.string_value;
        case ValueType::Boolean:
            return left.bool_value == right.bool_value;
        case ValueType::Object:
            return left.object_value == right.object_value;
        case ValueType::Vector:
            return left.vector_value == right.vector_value;
        case ValueType::Null:
            return true;
    }

    return false;
}

inline bool operator!=(const Value& left, const Value& right) {
    return !(left == right);
}

#endif
