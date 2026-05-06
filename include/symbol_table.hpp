#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "semantic_error.hpp"

class SymbolTable {
public:
    SymbolTable() { enter_scope(); }

    void enter_scope() { scopes_.emplace_back(); }

    void exit_scope() {
        if (scopes_.empty()) {
            throw std::runtime_error("SymbolTable: intento de salir de un scope vacío.");
        }
        scopes_.pop_back();
        if (scopes_.empty()) {
            scopes_.emplace_back();
        }
    }

    void define(const std::string& name, const std::string& type, int line) {
        auto& scope = scopes_.back();
        if (scope.count(name) > 0) {
            throw SemanticError(line, "símbolo duplicado en el mismo scope: " + name);
        }
        scope[name] = type;
    }

    std::string lookup(const std::string& name, int line) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        throw SemanticError(line, "símbolo no definido: " + name);
    }

    void assign(const std::string& name, const std::string& type, int line) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                found->second = type;
                return;
            }
        }
        throw SemanticError(line, "símbolo no definido: " + name);
    }

private:
    std::vector<std::unordered_map<std::string, std::string>> scopes_;
};

#endif
