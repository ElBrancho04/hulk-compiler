#ifndef VM_HPP
#define VM_HPP

#include <cstddef>
#include <memory>
#include <vector>

#include "bytecode.hpp"
#include "environment.hpp"
#include "value.hpp"

class VM {
public:
    struct CallFrame {
        std::size_t return_ip;
        std::shared_ptr<Environment> env;
        Value self;
    };

    VM();

    void execute(BytecodeProgram& program);

private:
    std::vector<Value> stack_;
    const std::vector<Value>* constants_ = nullptr;
    std::shared_ptr<Environment> global_env_;
    std::shared_ptr<Environment> current_env_;
    Value current_self_ = Value::Null();
    std::size_t ip_ = 0;
    std::vector<CallFrame> call_stack_;
};

#endif
