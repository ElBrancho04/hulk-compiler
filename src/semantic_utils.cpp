#include "semantic_utils.hpp"

int semantic_expr_line(const Expr* expr, int fallback) {
    if (expr) {
        return expr->line;
    }
    return fallback;
}

std::string require_inferred_type(const std::string& type,
                                  int line,
                                  const std::string& context) {
    if (type.empty()) {
        throw SemanticError(line, "no se pudo inferir tipo para " + context);
    }
    return type;
}

void require_annotation(const std::string& annotation,
                        int line,
                        const std::string& context) {
    if (annotation.empty()) {
        throw SemanticError(line, "anotación de tipo requerida para " + context);
    }
}
