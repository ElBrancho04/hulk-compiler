#ifndef SEMANTIC_CONTEXT_HPP
#define SEMANTIC_CONTEXT_HPP

#include <string>

struct SemanticContext {
    std::string current_type;
    std::string current_function;
    bool in_method = false;
    bool in_type = false;

    void enter_type(const std::string& type_name) {
        current_type = type_name;
        in_type = true;
    }

    void exit_type() {
        current_type.clear();
        in_type = false;
    }

    void enter_function(const std::string& func_name) {
        current_function = func_name;
        in_method = false;
    }

    void enter_method(const std::string& method_name) {
        current_function = method_name;
        in_method = true;
    }

    void exit_function() {
        current_function.clear();
        in_method = false;
    }
};

#endif
