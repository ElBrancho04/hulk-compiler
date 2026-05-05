#include "vm.hpp"

VM::VM()
    : global_env_(std::make_shared<Environment>()),
      current_env_(global_env_) {}

void VM::execute(BytecodeProgram& program) {
    constants_ = &program.constants;
    ip_ = 0;
    stack_.clear();
    call_stack_.clear();
    current_env_ = global_env_;
}
