#include "macro_expander.hpp"

#include <stdexcept>

// ============================================================
// Public entry point
// ============================================================

void MacroExpander::expand(Program& program) {
    // 1. Index all macro definitions.
    for (auto& m : program.macros) {
        if (!m) continue;
        if (macros_.count(m->name)) {
            throw std::runtime_error(
                "Duplicate macro definition: '" + m->name + "' line " +
                std::to_string(m->line));
        }
        macros_[m->name] = m.get();
    }

    // 2. Expand in function bodies.
    for (auto& f : program.functions) {
        if (f && f->body) {
            f->body = expand_expr(std::move(f->body));
        }
    }

    // 3. Expand in type method bodies and attribute initializers.
    for (auto& t : program.types) {
        if (!t) continue;
        for (auto& attr : t->attributes) {
            if (attr && attr->initializer) {
                attr->initializer = expand_expr(std::move(attr->initializer));
            }
        }
        for (auto& meth : t->methods) {
            if (meth && meth->body) {
                meth->body = expand_expr(std::move(meth->body));
            }
        }
        for (auto& parg : t->parent_args) {
            if (parg) parg = expand_expr(std::move(parg));
        }
    }

    // 4. Expand the global expression.
    if (program.global_expression) {
        program.global_expression = expand_expr(std::move(program.global_expression));
    }
}

// ============================================================
// Recursive walk: expand MacroInvoke and MatchExpr in-place
// ============================================================

std::unique_ptr<Expr> MacroExpander::expand_expr(std::unique_ptr<Expr> expr) {
    if (!expr) return nullptr;

    // --- Handle MacroInvoke ---
    if (auto* inv = dynamic_cast<MacroInvoke*>(expr.get())) {
        if (expansion_depth_ >= kMaxDepth) {
            throw std::runtime_error(
                "Macro expansion depth limit exceeded — recursive macro? '" +
                inv->name + "'");
        }
        ++expansion_depth_;
        auto expanded = expand_macro(*inv);
        --expansion_depth_;
        // Recursively expand the result (may contain nested macros).
        return expand_expr(std::move(expanded));
    }

    // --- Handle MatchExpr ---
    if (auto* match = dynamic_cast<MatchExpr*>(expr.get())) {
        auto desugared = desugar_match(*match);
        return expand_expr(std::move(desugared));
    }

    // --- Recurse into children of every other node type ---

    if (auto* n = dynamic_cast<BinaryExpr*>(expr.get())) {
        n->left  = expand_expr(std::move(n->left));
        n->right = expand_expr(std::move(n->right));
        return expr;
    }
    if (auto* n = dynamic_cast<UnaryExpr*>(expr.get())) {
        n->operand = expand_expr(std::move(n->operand));
        return expr;
    }
    if (auto* n = dynamic_cast<BlockExpr*>(expr.get())) {
        for (auto& e : n->expressions) e = expand_expr(std::move(e));
        return expr;
    }
    if (auto* n = dynamic_cast<AssignExpr*>(expr.get())) {
        n->value = expand_expr(std::move(n->value));
        return expr;
    }
    if (auto* n = dynamic_cast<LetExpr*>(expr.get())) {
        for (auto& b : n->bindings)
            if (b.initializer) b.initializer = expand_expr(std::move(b.initializer));
        n->body = expand_expr(std::move(n->body));
        return expr;
    }
    if (auto* n = dynamic_cast<IfExpr*>(expr.get())) {
        for (auto& br : n->branches) {
            br.condition = expand_expr(std::move(br.condition));
            br.body      = expand_expr(std::move(br.body));
        }
        n->else_body = expand_expr(std::move(n->else_body));
        return expr;
    }
    if (auto* n = dynamic_cast<WhileExpr*>(expr.get())) {
        n->condition = expand_expr(std::move(n->condition));
        n->body      = expand_expr(std::move(n->body));
        return expr;
    }
    if (auto* n = dynamic_cast<ForExpr*>(expr.get())) {
        n->iterable = expand_expr(std::move(n->iterable));
        n->body     = expand_expr(std::move(n->body));
        return expr;
    }
    if (auto* n = dynamic_cast<FuncCall*>(expr.get())) {
        // If the name matches a macro definition, treat as value-only MacroInvoke.
        if (macros_.count(n->name)) {
            std::vector<std::unique_ptr<Expr>> expanded_args;
            for (auto& a : n->args) expanded_args.push_back(expand_expr(std::move(a)));
            MacroInvoke inv(n->name, std::move(expanded_args), nullptr, n->line, n->col);
            return expand_expr(expand_macro(inv));
        }
        for (auto& a : n->args) a = expand_expr(std::move(a));
        return expr;
    }
    if (auto* n = dynamic_cast<NewExpr*>(expr.get())) {
        for (auto& a : n->args) a = expand_expr(std::move(a));
        return expr;
    }
    if (auto* n = dynamic_cast<MemberAccess*>(expr.get())) {
        n->object = expand_expr(std::move(n->object));
        return expr;
    }
    if (auto* n = dynamic_cast<MethodCall*>(expr.get())) {
        n->object = expand_expr(std::move(n->object));
        for (auto& a : n->args) a = expand_expr(std::move(a));
        return expr;
    }
    if (auto* n = dynamic_cast<BaseCall*>(expr.get())) {
        for (auto& a : n->args) a = expand_expr(std::move(a));
        return expr;
    }
    if (auto* n = dynamic_cast<IsExpr*>(expr.get())) {
        n->expression = expand_expr(std::move(n->expression));
        return expr;
    }
    if (auto* n = dynamic_cast<AsExpr*>(expr.get())) {
        n->expression = expand_expr(std::move(n->expression));
        return expr;
    }
    if (auto* n = dynamic_cast<VectorLiteral*>(expr.get())) {
        for (auto& e : n->elements) e = expand_expr(std::move(e));
        return expr;
    }
    if (auto* n = dynamic_cast<VectorComprehension*>(expr.get())) {
        n->generator = expand_expr(std::move(n->generator));
        n->iterable  = expand_expr(std::move(n->iterable));
        return expr;
    }
    if (auto* n = dynamic_cast<VectorComprehensionFilter*>(expr.get())) {
        n->generator = expand_expr(std::move(n->generator));
        n->iterable  = expand_expr(std::move(n->iterable));
        n->filter    = expand_expr(std::move(n->filter));
        return expr;
    }
    if (auto* n = dynamic_cast<VectorIndex*>(expr.get())) {
        n->vector = expand_expr(std::move(n->vector));
        n->index  = expand_expr(std::move(n->index));
        return expr;
    }
    if (auto* n = dynamic_cast<LambdaExpr*>(expr.get())) {
        n->body = expand_expr(std::move(n->body));
        return expr;
    }

    // Leaf nodes (NumberLiteral, StringLiteral, BoolLiteral, VarRef, SelfRef):
    // nothing to expand.
    return expr;
}

// ============================================================
// Macro expansion: clone body + substitute params
// ============================================================

std::unique_ptr<Expr> MacroExpander::expand_macro(MacroInvoke& inv) {
    auto it = macros_.find(inv.name);
    if (it == macros_.end()) {
        throw std::runtime_error(
            "Undefined macro '" + inv.name + "' at line " + std::to_string(inv.line));
    }
    MacroDef* def = it->second;

    // Count expected args: regular + syntactic (at most 1 syntactic).
    std::size_t reg_count = 0;
    bool has_syntactic = false;
    for (auto& p : def->params) {
        if (p.is_syntactic) has_syntactic = true;
        else ++reg_count;
    }

    if (inv.value_args.size() != reg_count) {
        throw std::runtime_error(
            "Macro '" + inv.name + "' expects " + std::to_string(reg_count) +
            " value argument(s), got " + std::to_string(inv.value_args.size()));
    }
    if (has_syntactic && !inv.block_arg) {
        throw std::runtime_error(
            "Macro '" + inv.name + "' requires a block argument { ... }");
    }
    if (!has_syntactic && inv.block_arg) {
        throw std::runtime_error(
            "Macro '" + inv.name + "' does not take a block argument");
    }

    // Build substitution maps.
    std::unordered_map<std::string, const Expr*> subs;
    std::unordered_map<std::string, std::string> gensyms;

    // For $placeholder hygiene: each expansion gets unique names.
    std::string prefix = "__m" + std::to_string(gensym_counter_++) + "_";

    std::size_t reg_idx = 0;
    for (auto& p : def->params) {
        if (p.is_syntactic) {
            subs[p.name] = inv.block_arg.get();
        } else {
            subs[p.name] = inv.value_args[reg_idx++].get();
        }
    }

    // Clone the macro body and substitute.
    auto body_clone = clone(def->body.get());
    return substitute(std::move(body_clone), subs, gensyms);
}

// ============================================================
// Match desugar: match(subject) { case T: body; ... default: d; }
//   →  let __m0__ = subject in
//      if (__m0__ is T) body  elif ...  else d
// ============================================================

std::unique_ptr<Expr> MacroExpander::desugar_match(MatchExpr& match) {
    std::string sv = fresh("__match");
    int ln = match.line;

    // Build the if/elif/else chain from arms (last to first for nesting).
    std::unique_ptr<Expr> else_part = match.default_body
        ? std::move(match.default_body)
        : std::make_unique<BoolLiteral>(false, ln);  // fallback if no default

    // Build branches list for IfExpr (each arm = one branch).
    std::vector<IfBranch> branches;
    for (auto& arm : match.arms) {
        // condition: __match is TypeName
        auto cond = std::make_unique<IsExpr>(
            std::make_unique<VarRef>(sv, arm.line), arm.type_name, arm.line);

        branches.emplace_back(std::move(cond), std::move(arm.body));
    }

    auto if_expr = std::make_unique<IfExpr>(
        std::move(branches), std::move(else_part), ln);

    // Wrap in: let __match = subject in <if_expr>
    std::vector<LetBinding> bindings;
    bindings.emplace_back(sv, "", std::move(match.subject));
    return std::make_unique<LetExpr>(std::move(bindings), std::move(if_expr), ln);
}

// ============================================================
// Deep clone
// ============================================================

std::unique_ptr<Expr> MacroExpander::clone(const Expr* e) const {
    if (!e) return nullptr;

    auto clv = [&](const std::vector<std::unique_ptr<Expr>>& v) {
        std::vector<std::unique_ptr<Expr>> out;
        for (auto& x : v) out.push_back(clone(x.get()));
        return out;
    };

    if (auto* n = dynamic_cast<const NumberLiteral*>(e))
        return std::make_unique<NumberLiteral>(n->value, n->line, n->col);
    if (auto* n = dynamic_cast<const StringLiteral*>(e))
        return std::make_unique<StringLiteral>(n->value, n->line, n->col);
    if (auto* n = dynamic_cast<const BoolLiteral*>(e))
        return std::make_unique<BoolLiteral>(n->value, n->line, n->col);
    if (auto* n = dynamic_cast<const VarRef*>(e))
        return std::make_unique<VarRef>(n->name, n->line, n->col);
    if (auto* n = dynamic_cast<const SelfRef*>(e))
        return std::make_unique<SelfRef>(n->line, n->col);

    if (auto* n = dynamic_cast<const BinaryExpr*>(e))
        return std::make_unique<BinaryExpr>(n->op, clone(n->left.get()), clone(n->right.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const UnaryExpr*>(e))
        return std::make_unique<UnaryExpr>(n->op, clone(n->operand.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const BlockExpr*>(e))
        return std::make_unique<BlockExpr>(clv(n->expressions), n->line, n->col);
    if (auto* n = dynamic_cast<const AssignExpr*>(e))
        return std::make_unique<AssignExpr>(n->name, clone(n->value.get()), n->line, n->col);

    if (auto* n = dynamic_cast<const LetExpr*>(e)) {
        std::vector<LetBinding> binds;
        for (auto& b : n->bindings)
            binds.emplace_back(b.name, b.type_annotation, clone(b.initializer.get()));
        return std::make_unique<LetExpr>(std::move(binds), clone(n->body.get()), n->line, n->col);
    }
    if (auto* n = dynamic_cast<const IfExpr*>(e)) {
        std::vector<IfBranch> branches;
        for (auto& br : n->branches)
            branches.emplace_back(clone(br.condition.get()), clone(br.body.get()));
        return std::make_unique<IfExpr>(std::move(branches), clone(n->else_body.get()), n->line, n->col);
    }
    if (auto* n = dynamic_cast<const WhileExpr*>(e))
        return std::make_unique<WhileExpr>(clone(n->condition.get()), clone(n->body.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const ForExpr*>(e))
        return std::make_unique<ForExpr>(n->variable_name, clone(n->iterable.get()), clone(n->body.get()), n->line, n->col);

    if (auto* n = dynamic_cast<const FuncCall*>(e)) {
        auto fc = std::make_unique<FuncCall>(n->name, clv(n->args), n->line, n->col);
        fc->is_functor = n->is_functor;
        return fc;
    }
    if (auto* n = dynamic_cast<const NewExpr*>(e))
        return std::make_unique<NewExpr>(n->type_name, clv(n->args), n->line, n->col);
    if (auto* n = dynamic_cast<const MemberAccess*>(e))
        return std::make_unique<MemberAccess>(clone(n->object.get()), n->member_name, n->line, n->col);
    if (auto* n = dynamic_cast<const MethodCall*>(e))
        return std::make_unique<MethodCall>(clone(n->object.get()), n->method_name, clv(n->args), n->line, n->col);
    if (auto* n = dynamic_cast<const BaseCall*>(e))
        return std::make_unique<BaseCall>(clv(n->args), n->line, n->col);
    if (auto* n = dynamic_cast<const IsExpr*>(e))
        return std::make_unique<IsExpr>(clone(n->expression.get()), n->type_name, n->line, n->col);
    if (auto* n = dynamic_cast<const AsExpr*>(e))
        return std::make_unique<AsExpr>(clone(n->expression.get()), n->type_name, n->line, n->col);
    if (auto* n = dynamic_cast<const VectorLiteral*>(e))
        return std::make_unique<VectorLiteral>(clv(n->elements), n->line, n->col);
    if (auto* n = dynamic_cast<const VectorComprehension*>(e))
        return std::make_unique<VectorComprehension>(
            clone(n->generator.get()), n->variable_name, clone(n->iterable.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const VectorComprehensionFilter*>(e))
        return std::make_unique<VectorComprehensionFilter>(
            clone(n->generator.get()), n->variable_name,
            clone(n->iterable.get()), clone(n->filter.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const VectorIndex*>(e))
        return std::make_unique<VectorIndex>(clone(n->vector.get()), clone(n->index.get()), n->line, n->col);
    if (auto* n = dynamic_cast<const LambdaExpr*>(e)) {
        auto lam = std::make_unique<LambdaExpr>(n->params, n->return_type, clone(n->body.get()), n->line, n->col);
        lam->generated_type_name = n->generated_type_name;
        lam->captured_vars = n->captured_vars;
        return lam;
    }
    // MacroInvoke and MatchExpr can appear in macro bodies (nested macros).
    if (auto* n = dynamic_cast<const MacroInvoke*>(e)) {
        auto cloned_block = n->block_arg ? clone(n->block_arg.get()) : nullptr;
        return std::make_unique<MacroInvoke>(n->name, clv(n->value_args), std::move(cloned_block), n->line, n->col);
    }
    if (auto* n = dynamic_cast<const MatchExpr*>(e)) {
        std::vector<MatchArm> arms;
        for (auto& a : n->arms)
            arms.emplace_back(a.type_name, clone(a.body.get()), a.line);
        auto def = n->default_body ? clone(n->default_body.get()) : nullptr;
        return std::make_unique<MatchExpr>(clone(n->subject.get()), std::move(arms), std::move(def), n->line, n->col);
    }

    throw std::runtime_error("MacroExpander::clone: unknown Expr subtype");
}

// ============================================================
// Substitution: replace VarRef(param) → clone(arg)
//               rename $name → gensym
// ============================================================

std::unique_ptr<Expr> MacroExpander::substitute(
        std::unique_ptr<Expr> expr,
        const std::unordered_map<std::string, const Expr*>& subs,
        const std::unordered_map<std::string, std::string>& gensyms) {

    if (!expr) return nullptr;

    // VarRef: check for substitution or $placeholder rename.
    if (auto* vr = dynamic_cast<VarRef*>(expr.get())) {
        // $name hygiene: rename to gensym.
        if (!vr->name.empty() && vr->name[0] == '$') {
            std::string key = vr->name.substr(1);
            auto git = gensyms.find(key);
            if (git != gensyms.end()) {
                return std::make_unique<VarRef>(git->second, vr->line, vr->col);
            }
            // Not pre-declared — create a new gensym entry on the fly.
            // (Callers pass gensyms by const ref; we just keep the $ as-is
            //  to signal "not yet resolved" — the expander will handle it.)
        }
        // Regular param substitution.
        auto it = subs.find(vr->name);
        if (it != subs.end()) {
            return clone(it->second);
        }
        return expr;
    }

    // AssignExpr: also rename $name on the lhs.
    if (auto* ae = dynamic_cast<AssignExpr*>(expr.get())) {
        ae->value = substitute(std::move(ae->value), subs, gensyms);
        if (!ae->name.empty() && ae->name[0] == '$') {
            std::string key = ae->name.substr(1);
            auto git = gensyms.find(key);
            if (git != gensyms.end()) ae->name = git->second;
        }
        return expr;
    }

    // Recurse into all compound nodes.
    if (auto* n = dynamic_cast<BinaryExpr*>(expr.get())) {
        n->left  = substitute(std::move(n->left),  subs, gensyms);
        n->right = substitute(std::move(n->right), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<UnaryExpr*>(expr.get())) {
        n->operand = substitute(std::move(n->operand), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<BlockExpr*>(expr.get())) {
        for (auto& e : n->expressions) e = substitute(std::move(e), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<LetExpr*>(expr.get())) {
        for (auto& b : n->bindings)
            if (b.initializer) b.initializer = substitute(std::move(b.initializer), subs, gensyms);
        n->body = substitute(std::move(n->body), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<IfExpr*>(expr.get())) {
        for (auto& br : n->branches) {
            br.condition = substitute(std::move(br.condition), subs, gensyms);
            br.body      = substitute(std::move(br.body),      subs, gensyms);
        }
        n->else_body = substitute(std::move(n->else_body), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<WhileExpr*>(expr.get())) {
        n->condition = substitute(std::move(n->condition), subs, gensyms);
        n->body      = substitute(std::move(n->body),      subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<ForExpr*>(expr.get())) {
        n->iterable = substitute(std::move(n->iterable), subs, gensyms);
        n->body     = substitute(std::move(n->body),     subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<FuncCall*>(expr.get())) {
        for (auto& a : n->args) a = substitute(std::move(a), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<NewExpr*>(expr.get())) {
        for (auto& a : n->args) a = substitute(std::move(a), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<MemberAccess*>(expr.get())) {
        n->object = substitute(std::move(n->object), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<MethodCall*>(expr.get())) {
        n->object = substitute(std::move(n->object), subs, gensyms);
        for (auto& a : n->args) a = substitute(std::move(a), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<BaseCall*>(expr.get())) {
        for (auto& a : n->args) a = substitute(std::move(a), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<IsExpr*>(expr.get())) {
        n->expression = substitute(std::move(n->expression), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<AsExpr*>(expr.get())) {
        n->expression = substitute(std::move(n->expression), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<VectorLiteral*>(expr.get())) {
        for (auto& e : n->elements) e = substitute(std::move(e), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<VectorComprehension*>(expr.get())) {
        n->generator = substitute(std::move(n->generator), subs, gensyms);
        n->iterable  = substitute(std::move(n->iterable),  subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<VectorComprehensionFilter*>(expr.get())) {
        n->generator = substitute(std::move(n->generator), subs, gensyms);
        n->iterable  = substitute(std::move(n->iterable),  subs, gensyms);
        n->filter    = substitute(std::move(n->filter),    subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<VectorIndex*>(expr.get())) {
        n->vector = substitute(std::move(n->vector), subs, gensyms);
        n->index  = substitute(std::move(n->index),  subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<LambdaExpr*>(expr.get())) {
        n->body = substitute(std::move(n->body), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<MacroInvoke*>(expr.get())) {
        for (auto& a : n->value_args) a = substitute(std::move(a), subs, gensyms);
        if (n->block_arg) n->block_arg = substitute(std::move(n->block_arg), subs, gensyms);
        return expr;
    }
    if (auto* n = dynamic_cast<MatchExpr*>(expr.get())) {
        n->subject = substitute(std::move(n->subject), subs, gensyms);
        for (auto& arm : n->arms)
            arm.body = substitute(std::move(arm.body), subs, gensyms);
        if (n->default_body)
            n->default_body = substitute(std::move(n->default_body), subs, gensyms);
        return expr;
    }

    // Leaf nodes: nothing to substitute.
    return expr;
}

// ============================================================
// Helpers
// ============================================================

std::string MacroExpander::fresh(const std::string& hint) {
    return hint + std::to_string(gensym_counter_++) + "__";
}
