#include <iostream>
#include <memory>
#include <vector>

#include "builtins.hpp"
#include "environment.hpp"
#include "hulk_vector.hpp"
#include "value_string.hpp"

namespace {
Value call_builtin(const std::shared_ptr<Environment>& env,
                   const std::string& name,
                   const std::vector<Value>& args) {
    Value fn_value = env->get(name);
    if (fn_value.type != ValueType::Object || !fn_value.object_value) {
        throw RuntimeError("Symbol is not a builtin function: " + name);
    }

    auto native_fn = std::dynamic_pointer_cast<NativeFunctionObject>(fn_value.object_value);
    if (!native_fn) {
        throw RuntimeError("Symbol is not a native builtin: " + name);
    }

    return native_fn->function(args);
}

bool expect_runtime_error(const std::string& label, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RuntimeError&) {
        std::cout << "[OK] " << label << " produced RuntimeError" << std::endl;
        return true;
    }

    std::cerr << "[FAIL] " << label << " did not throw RuntimeError" << std::endl;
    return false;
}
} // namespace

int main() {
    try {
        auto env = create_global_env();

        std::cout << "--- Builtins ---" << std::endl;
        call_builtin(env, "print", {Value::Number(1.0)});
        Value sqrt_val = call_builtin(env, "sqrt", {Value::Number(4.0)});
        std::cout << "sqrt(4) = " << to_string(sqrt_val) << std::endl;
        Value rand_val = call_builtin(env, "rand", {});
        std::cout << "rand() = " << to_string(rand_val) << std::endl;

        std::cout << "--- to_string ---" << std::endl;
        std::cout << to_string(Value::Number(3.5)) << std::endl;
        std::cout << to_string(Value::String("hola")) << std::endl;
        std::cout << to_string(Value::Boolean(true)) << std::endl;
        std::cout << to_string(Value::Null()) << std::endl;

        auto vector = std::make_shared<HulkVector>(std::vector<Value>{
            Value::Number(1.0),
            Value::String("dos"),
            Value::Boolean(false)
        });
        std::cout << to_string(Value::Vector(vector)) << std::endl;

        std::cout << "--- Environment errors ---" << std::endl;
        bool get_ok = expect_runtime_error("env.get", [&]() { env->get("missing"); });
        bool assign_ok = expect_runtime_error("env.assign", [&]() { env->assign("missing", Value::Null()); });

        if (!get_ok || !assign_ok) {
            return 1;
        }

        std::cout << "All runtime checks passed." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Runtime test failed: " << ex.what() << std::endl;
        return 1;
    }
}
