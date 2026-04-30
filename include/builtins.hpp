#ifndef BUILTINS_HPP
#define BUILTINS_HPP

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "environment.hpp"
#include "hulk_object.hpp"
#include "hulk_range.hpp"
#include "runtime_error.hpp"
#include "value_string.hpp"

using NativeFunction = std::function<Value(const std::vector<Value>&)>;

class NativeFunctionObject : public HulkObject {
public:
    NativeFunction function;

    NativeFunctionObject(std::string name, NativeFunction function)
        : HulkObject(std::move(name)), function(std::move(function)) {}
};

inline void ensure_arg_count(const std::string& name, const std::vector<Value>& args, std::size_t expected) {
    if (args.size() != expected) {
        throw RuntimeError("Builtin " + name + " expects " + std::to_string(expected) + " args");
    }
}

inline double expect_number(const std::string& name, const Value& value) {
    if (value.type != ValueType::Number) {
        throw RuntimeError("Builtin " + name + " expects Number arguments");
    }
    return value.number_value;
}

inline std::shared_ptr<Environment> create_global_env() {
    auto env = std::make_shared<Environment>();

    env->define("PI", Value::Number(3.14159265358979323846));
    env->define("E", Value::Number(2.71828182845904523536));

    env->define("print", Value::Object(std::make_shared<NativeFunctionObject>("builtin:print",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("print", args, 1);
            std::cout << to_string(args[0]) << std::endl;
            return Value::Null();
        }
    )));

    env->define("sqrt", Value::Object(std::make_shared<NativeFunctionObject>("builtin:sqrt",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("sqrt", args, 1);
            return Value::Number(std::sqrt(expect_number("sqrt", args[0])));
        }
    )));

    env->define("sin", Value::Object(std::make_shared<NativeFunctionObject>("builtin:sin",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("sin", args, 1);
            return Value::Number(std::sin(expect_number("sin", args[0])));
        }
    )));

    env->define("cos", Value::Object(std::make_shared<NativeFunctionObject>("builtin:cos",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("cos", args, 1);
            return Value::Number(std::cos(expect_number("cos", args[0])));
        }
    )));

    env->define("exp", Value::Object(std::make_shared<NativeFunctionObject>("builtin:exp",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("exp", args, 1);
            return Value::Number(std::exp(expect_number("exp", args[0])));
        }
    )));

    env->define("log", Value::Object(std::make_shared<NativeFunctionObject>("builtin:log",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("log", args, 2);
            double base = expect_number("log", args[0]);
            double value = expect_number("log", args[1]);
            if (base <= 0.0 || base == 1.0 || value <= 0.0) {
                throw RuntimeError("Builtin log expects base > 0, base != 1, value > 0");
            }
            return Value::Number(std::log(value) / std::log(base));
        }
    )));

    env->define("rand", Value::Object(std::make_shared<NativeFunctionObject>("builtin:rand",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("rand", args, 0);
            static thread_local std::mt19937 generator{std::random_device{}()};
            static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
            return Value::Number(distribution(generator));
        }
    )));

    env->define("range", Value::Object(std::make_shared<NativeFunctionObject>("builtin:range",
        [](const std::vector<Value>& args) -> Value {
            ensure_arg_count("range", args, 2);
            double start = expect_number("range", args[0]);
            double end = expect_number("range", args[1]);
            return Value::Object(std::make_shared<HulkRange>(start, end));
        }
    )));

    return env;
}

#endif
