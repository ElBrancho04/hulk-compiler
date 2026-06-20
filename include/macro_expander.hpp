#ifndef MACRO_EXPANDER_HPP
#define MACRO_EXPANDER_HPP

#include <string>
#include <unordered_map>

#include "ast.hpp"

// MacroExpander: AST-to-AST pass that must run BEFORE SemanticAnalyzer.
//
// Responsibilities:
//   1. Collect MacroDef nodes from Program.macros into a lookup table.
//   2. Walk every expression in the Program and expand MacroInvoke nodes
//      by cloning the macro body and substituting parameters.
//   3. Desugar MatchExpr nodes into if/is/else chains.
//   4. Rename $placeholder variables to unique gensyms for basic hygiene.
//
// After expand() returns, no MacroInvoke or MatchExpr remain in the tree.
class MacroExpander {
public:
    void expand(Program& program);

private:
    std::unordered_map<std::string, MacroDef*> macros_;
    int gensym_counter_ = 0;
    int expansion_depth_ = 0;
    static constexpr int kMaxDepth = 64;

    // --- Main recursive walk ---
    std::unique_ptr<Expr> expand_expr(std::unique_ptr<Expr> expr);

    // --- Macro expansion ---
    std::unique_ptr<Expr> expand_macro(MacroInvoke& inv);

    // --- Match desugar ---
    std::unique_ptr<Expr> desugar_match(MatchExpr& match);

    // --- Deep clone of any Expr node ---
    std::unique_ptr<Expr> clone(const Expr* expr) const;

    // --- Substitution: replace VarRefs named in `subs` with clones of their replacements.
    //     Also replaces $name VarRefs with fresh gensym names using `prefix`. ---
    std::unique_ptr<Expr> substitute(std::unique_ptr<Expr> expr,
                                     const std::unordered_map<std::string, const Expr*>& subs,
                                     std::unordered_map<std::string, std::string>& gensyms,
                                     const std::string& prefix);

    std::string fresh(const std::string& hint);
};

#endif
