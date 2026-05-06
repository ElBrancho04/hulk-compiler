#include "semantic_analyzer.hpp"

#include <unordered_set>

namespace {
constexpr const char* kObjectType = "Object";
constexpr const char* kNumberType = "Number";
constexpr const char* kStringType = "String";
constexpr const char* kBooleanType = "Boolean";

std::string normalize_parent(const std::string& parent) {
    return parent.empty() ? std::string(kObjectType) : parent;
}
} // namespace

void SemanticAnalyzer::analyze(Program& program) {
    pass1_register_types(program);
    pass2_register_functions(program);
    pass3_type_check(program);
}

void SemanticAnalyzer::pass1_register_types(Program& program) {
    std::unordered_map<std::string, std::string> parents;
    std::unordered_map<std::string, TypeInfo> pending;
    std::unordered_set<std::string> seen_types;

    for (const auto& type_def : program.types) {
        if (!type_def) {
            continue;
        }
        const std::string& name = type_def->name;
        if (seen_types.count(name) > 0) {
            throw SemanticError(type_def->line, "tipo duplicado: " + name);
        }
        seen_types.insert(name);

        std::unordered_set<std::string> attr_names;
        std::unordered_set<std::string> method_names;
        std::unordered_map<std::string, std::string> attrs;
        std::unordered_map<std::string, MethodSig> methods;

        for (const auto& attr : type_def->attributes) {
            if (!attr) {
                continue;
            }
            if (attr_names.count(attr->name) > 0) {
                throw SemanticError(attr->line, "atributo duplicado en tipo " + name + ": " + attr->name);
            }
            attr_names.insert(attr->name);
            attrs.emplace(attr->name, attr->type_annotation);
        }

        for (const auto& method : type_def->methods) {
            if (!method) {
                continue;
            }
            if (method_names.count(method->name) > 0) {
                throw SemanticError(method->line, "método duplicado en tipo " + name + ": " + method->name);
            }
            method_names.insert(method->name);
            validate_no_duplicates(method->params, method->line, "método " + method->name);

            std::vector<std::string> param_types;
            param_types.reserve(method->params.size());
            for (const auto& param : method->params) {
                param_types.push_back(param.type_annotation);
            }
            methods.emplace(method->name, MethodSig{std::move(param_types), method->return_type});
        }

        const std::string parent_name = normalize_parent(type_def->parent_name);
        parents.emplace(name, parent_name);
        pending.emplace(name, TypeInfo{name, parent_name, std::move(attrs), std::move(methods)});
    }

    register_protocols(program.protocols);
    validate_no_cycles(parents);

    bool progress = true;
    while (!pending.empty() && progress) {
        progress = false;
        for (auto it = pending.begin(); it != pending.end();) {
            const auto& type_info = it->second;
            if (type_table_.has_type(type_info.parent)) {
                type_table_.register_type(type_info);
                it = pending.erase(it);
                progress = true;
            } else {
                ++it;
            }
        }
    }

    if (!pending.empty()) {
        const auto& first = pending.begin()->second;
        throw SemanticError(0, "tipo padre inexistente o ciclo detectado: " + first.parent);
    }
}

void SemanticAnalyzer::pass2_register_functions(Program& program) {
    for (const auto& func_def : program.functions) {
        if (!func_def) {
            continue;
        }
        if (functions_.count(func_def->name) > 0) {
            throw SemanticError(func_def->line, "función duplicada: " + func_def->name);
        }
        validate_no_duplicates(func_def->params, func_def->line, "función " + func_def->name);

        std::vector<std::string> param_types;
        param_types.reserve(func_def->params.size());
        for (const auto& param : func_def->params) {
            param_types.push_back(param.type_annotation);
        }
        functions_.emplace(func_def->name, FunctionSig{std::move(param_types), func_def->return_type});
    }
}

void SemanticAnalyzer::pass3_type_check(Program& program) {
    symbols_ = SymbolTable();
    context_ = SemanticContext();

    for (const auto& type_def : program.types) {
        if (type_def) {
            visit(*type_def);
        }
    }

    for (const auto& func_def : program.functions) {
        if (func_def) {
            visit(*func_def);
        }
    }

    if (program.global_expression) {
        analyze_expr(program.global_expression.get());
    }
}

void SemanticAnalyzer::register_protocols(const std::vector<std::unique_ptr<ProtocolDef>>& protocols) {
    for (const auto& proto : protocols) {
        if (!proto) {
            continue;
        }
        if (protocols_.count(proto->name) > 0) {
            throw SemanticError(proto->line, "protocolo duplicado: " + proto->name);
        }

        ProtocolInfo info;
        info.name = proto->name;
        info.parent = proto->parent_name;

        std::unordered_set<std::string> method_names;
        for (const auto& method : proto->methods) {
            if (!method) {
                continue;
            }
            if (method_names.count(method->name) > 0) {
                throw SemanticError(method->line, "método duplicado en protocolo " + info.name + ": " + method->name);
            }
            method_names.insert(method->name);
            validate_no_duplicates(method->params, method->line, "protocolo " + info.name);

            std::vector<std::string> param_types;
            param_types.reserve(method->params.size());
            for (const auto& param : method->params) {
                param_types.push_back(param.type_annotation);
            }
            info.methods.emplace(method->name, MethodSig{std::move(param_types), method->return_type});
        }

        protocols_.emplace(info.name, std::move(info));
    }
}

void SemanticAnalyzer::validate_no_cycles(const std::unordered_map<std::string, std::string>& parents) {
    enum class Mark { Unvisited, Visiting, Done };
    std::unordered_map<std::string, Mark> marks;

    auto visit_node = [&](const std::string& name, const auto& self) -> void {
        auto mark_it = marks.find(name);
        if (mark_it != marks.end()) {
            if (mark_it->second == Mark::Visiting) {
                throw SemanticError(0, "ciclo en herencia detectado en tipo: " + name);
            }
            if (mark_it->second == Mark::Done) {
                return;
            }
        }

        marks[name] = Mark::Visiting;
        auto parent_it = parents.find(name);
        if (parent_it != parents.end()) {
            const std::string& parent = parent_it->second;
            if (parent != kObjectType && parents.count(parent) == 0) {
                throw SemanticError(0, "tipo padre inexistente: " + parent);
            }
            if (parent != kObjectType) {
                self(parent, self);
            }
        }
        marks[name] = Mark::Done;
    };

    for (const auto& entry : parents) {
        if (marks[entry.first] == Mark::Unvisited) {
            visit_node(entry.first, visit_node);
        }
    }
}

void SemanticAnalyzer::validate_no_duplicates(const std::vector<Parameter>& params, int line, const std::string& owner) {
    std::unordered_set<std::string> names;
    for (const auto& param : params) {
        if (names.count(param.name) > 0) {
            throw SemanticError(line, "parámetro duplicado en " + owner + ": " + param.name);
        }
        names.insert(param.name);
    }
}

std::string SemanticAnalyzer::visit(NumberLiteral& node) {
    (void)node;
    return kNumberType;
}

std::string SemanticAnalyzer::visit(StringLiteral& node) {
    (void)node;
    return kStringType;
}

std::string SemanticAnalyzer::visit(BoolLiteral& node) {
    (void)node;
    return kBooleanType;
}

std::string SemanticAnalyzer::visit(BinaryExpr& node) {
    analyze_expr(node.left.get());
    analyze_expr(node.right.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(UnaryExpr& node) {
    analyze_expr(node.operand.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(BlockExpr& node) {
    std::string result = kObjectType;
    for (auto& expr : node.expressions) {
        result = analyze_expr(expr.get());
    }
    return result;
}

std::string SemanticAnalyzer::visit(VarRef& node) {
    return symbols_.lookup(node.name, node.line);
}

std::string SemanticAnalyzer::visit(AssignExpr& node) {
    analyze_expr(node.value.get());
    return symbols_.lookup(node.name, node.line);
}

std::string SemanticAnalyzer::visit(LetBinding& node) {
    analyze_expr(node.initializer.get());
    symbols_.define(node.name, node.type_annotation.empty() ? kObjectType : node.type_annotation, 0);
    return symbols_.lookup(node.name, 0);
}

std::string SemanticAnalyzer::visit(LetExpr& node) {
    symbols_.enter_scope();
    for (auto& binding : node.bindings) {
        visit(binding);
    }
    std::string result = kObjectType;
    result = analyze_expr(node.body.get());
    symbols_.exit_scope();
    return result;
}

std::string SemanticAnalyzer::visit(IfExpr& node) {
    for (auto& branch : node.branches) {
        analyze_expr(branch.condition.get());
        analyze_expr(branch.body.get());
    }
    analyze_expr(node.else_body.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(WhileExpr& node) {
    analyze_expr(node.condition.get());
    analyze_expr(node.body.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(ForExpr& node) {
    analyze_expr(node.iterable.get());
    symbols_.enter_scope();
    symbols_.define(node.variable_name, kObjectType, node.line);
    analyze_expr(node.body.get());
    symbols_.exit_scope();
    return kObjectType;
}

std::string SemanticAnalyzer::visit(FuncCall& node) {
    for (auto& arg : node.args) {
        analyze_expr(arg.get());
    }
    auto it = functions_.find(node.name);
    if (it != functions_.end()) {
        return it->second.return_type.empty() ? kObjectType : it->second.return_type;
    }
    return kObjectType;
}

std::string SemanticAnalyzer::visit(FuncDef& node) {
    context_.enter_function(node.name);
    symbols_.enter_scope();
    for (const auto& param : node.params) {
        symbols_.define(param.name, param.type_annotation.empty() ? kObjectType : param.type_annotation, node.line);
    }
    analyze_expr(node.body.get());
    symbols_.exit_scope();
    context_.exit_function();
    return kObjectType;
}

std::string SemanticAnalyzer::visit(TypeDef& node) {
    context_.enter_type(node.name);
    for (const auto& attr : node.attributes) {
        if (attr && attr->initializer) {
            symbols_.enter_scope();
            analyze_expr(attr->initializer.get());
            symbols_.exit_scope();
        }
    }

    for (const auto& method : node.methods) {
        if (!method) {
            continue;
        }
        context_.enter_method(method->name);
        symbols_.enter_scope();
        for (const auto& param : method->params) {
            symbols_.define(param.name, param.type_annotation.empty() ? kObjectType : param.type_annotation, method->line);
        }
        analyze_expr(method->body.get());
        symbols_.exit_scope();
        context_.exit_function();
    }

    context_.exit_type();
    return kObjectType;
}

std::string SemanticAnalyzer::visit(ProtocolDef& node) {
    (void)node;
    return kObjectType;
}

std::string SemanticAnalyzer::visit(ProtocolMethodSig& node) {
    (void)node;
    return kObjectType;
}

std::string SemanticAnalyzer::visit(NewExpr& node) {
    for (auto& arg : node.args) {
        analyze_expr(arg.get());
    }
    return node.type_name.empty() ? kObjectType : node.type_name;
}

std::string SemanticAnalyzer::visit(MemberAccess& node) {
    analyze_expr(node.object.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(MethodCall& node) {
    analyze_expr(node.object.get());
    for (auto& arg : node.args) {
        analyze_expr(arg.get());
    }
    return kObjectType;
}

std::string SemanticAnalyzer::visit(SelfRef& node) {
    (void)node;
    return context_.current_type.empty() ? kObjectType : context_.current_type;
}

std::string SemanticAnalyzer::visit(BaseCall& node) {
    for (auto& arg : node.args) {
        analyze_expr(arg.get());
    }
    return kObjectType;
}

std::string SemanticAnalyzer::visit(IsExpr& node) {
    analyze_expr(node.expression.get());
    return kBooleanType;
}

std::string SemanticAnalyzer::visit(AsExpr& node) {
    analyze_expr(node.expression.get());
    return node.type_name.empty() ? kObjectType : node.type_name;
}

std::string SemanticAnalyzer::visit(VectorLiteral& node) {
    for (auto& expr : node.elements) {
        analyze_expr(expr.get());
    }
    type_table_.ensure_vector_type(kObjectType);
    return TypeTable::make_vector_type(kObjectType);
}

std::string SemanticAnalyzer::visit(VectorComprehension& node) {
    analyze_expr(node.iterable.get());
    symbols_.enter_scope();
    symbols_.define(node.variable_name, kObjectType, 0);
    analyze_expr(node.generator.get());
    symbols_.exit_scope();
    type_table_.ensure_vector_type(kObjectType);
    return TypeTable::make_vector_type(kObjectType);
}

std::string SemanticAnalyzer::visit(VectorComprehensionFilter& node) {
    analyze_expr(node.iterable.get());
    symbols_.enter_scope();
    symbols_.define(node.variable_name, kObjectType, 0);
    analyze_expr(node.generator.get());
    analyze_expr(node.filter.get());
    symbols_.exit_scope();
    type_table_.ensure_vector_type(kObjectType);
    return TypeTable::make_vector_type(kObjectType);
}

std::string SemanticAnalyzer::visit(VectorIndex& node) {
    analyze_expr(node.vector.get());
    analyze_expr(node.index.get());
    return kObjectType;
}

std::string SemanticAnalyzer::visit(Program& node) {
    analyze(node);
    return kObjectType;
}

std::string SemanticAnalyzer::analyze_expr(Expr* expr) {
    if (!expr) {
        return kObjectType;
    }
    if (auto* node = dynamic_cast<NumberLiteral*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<StringLiteral*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<BoolLiteral*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<BinaryExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<UnaryExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<BlockExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<VarRef*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<AssignExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<LetExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<IfExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<WhileExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<ForExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<FuncCall*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<NewExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<MemberAccess*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<MethodCall*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<SelfRef*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<BaseCall*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<IsExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<AsExpr*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<VectorLiteral*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<VectorComprehension*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<VectorComprehensionFilter*>(expr)) {
        return visit(*node);
    }
    if (auto* node = dynamic_cast<VectorIndex*>(expr)) {
        return visit(*node);
    }
    return kObjectType;
}
