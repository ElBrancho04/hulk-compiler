#ifndef VALUE_STRING_HPP
#define VALUE_STRING_HPP

#include <sstream>
#include <string>

#include "hulk_object.hpp"
#include "hulk_vector.hpp"
#include "value.hpp"

inline std::string to_string(const Value& value) {
    switch (value.type) {
        case ValueType::Number: {
            std::ostringstream out;
            out << value.number_value;
            return out.str();
        }
        case ValueType::String:
            return value.string_value;
        case ValueType::Boolean:
            return value.bool_value ? "true" : "false";
        case ValueType::Null:
            return "null";
        case ValueType::Vector: {
            if (!value.vector_value) {
                return "[]";
            }
            std::ostringstream out;
            out << "[";
            for (std::size_t i = 0; i < value.vector_value->elements.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << to_string(value.vector_value->elements[i]);
            }
            out << "]";
            return out.str();
        }
        case ValueType::Object: {
            if (!value.object_value) {
                return "<null>";
            }
            return "<" + value.object_value->type_name + ">";
        }
    }

    return "null";
}

#endif
