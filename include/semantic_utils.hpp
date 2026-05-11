#ifndef SEMANTIC_UTILS_HPP
#define SEMANTIC_UTILS_HPP

#include <string>

#include "ast.hpp"
#include "semantic_error.hpp"

int semantic_expr_line(const Expr* expr, int fallback);

std::string require_inferred_type(const std::string& type,
                                  int line,
                                  const std::string& context);

void require_annotation(const std::string& annotation,
                        int line,
                        const std::string& context);

#endif
