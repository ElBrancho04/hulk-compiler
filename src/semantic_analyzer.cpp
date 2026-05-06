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
    register_builtins();
    pass1_register_types(program);
    pass2_register_functions(program);
    pass3_type_check(program);
}

SemanticAnalyzer::SemanticAnalyzer() {
    register_builtins();
}

void SemanticAnalyzer::register_builtins() {
    if (!functions_.empty()) {
        return;
    }

    functions_.emplace("print", FunctionSig{{kObjectType}, kObjectType});
    functions_.emplace("sqrt", FunctionSig{{kNumberType}, kNumberType});
    functions_.emplace("sin", FunctionSig{{kNumberType}, kNumberType});
    functions_.emplace("cos", FunctionSig{{kNumberType}, kNumberType});
    functions_.emplace("exp", FunctionSig{{kNumberType}, kNumberType});
    functions_.emplace("log", FunctionSig{{kNumberType, kNumberType}, kNumberType});
    functions_.emplace("rand", FunctionSig{{}, kNumberType});

    type_table_.ensure_iterable_type(kNumberType);
    functions_.emplace("range", FunctionSig{{kNumberType, kNumberType}, TypeTable::make_iterable_type(kNumberType)});
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

        std::vector<std::string> ctor_params;
        ctor_params.reserve(type_def->type_params.size());
        for (const auto& param : type_def->type_params) {
            ctor_params.push_back(param.type_annotation.empty() ? kObjectType : param.type_annotation);
        }
        type_constructors_[name] = std::move(ctor_params);

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
            param_types.push_back(param.type_annotation.empty() ? kObjectType : param.type_annotation);
        }
        functions_.emplace(func_def->name, FunctionSig{std::move(param_types), func_def->return_type});
    }
}

void SemanticAnalyzer::pass3_type_check(Program& program) {
    symbols_ = SymbolTable();
    context_ = SemanticContext();
    analyzed_types_.clear();

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

void SemanticAnalyzer::ensure_conforms(const std::string& actual,
                                       const std::string& expected,
                                       int line,
                                       const std::string& context) {
    if (actual.empty() || expected.empty()) {
        return;
    }
    if (!type_table_.conforms_to(actual, expected)) {
        throw SemanticError(line, "tipo incompatible en " + context + ": se esperaba " + expected + ", se obtuvo " + actual);
    }
}

void SemanticAnalyzer::ensure_equals_or_conforms(const std::string& left,
                                                 const std::string& right,
                                                 int line,
                                                 const std::string& context) {
    if (left.empty() || right.empty()) {
        return;
    }
    if (left == right) {
        return;
    }
    if (type_table_.conforms_to(left, right) || type_table_.conforms_to(right, left)) {
        return;
    }
    throw SemanticError(line, "tipos incompatibles en " + context + ": " + left + " y " + right);
}

std::string SemanticAnalyzer::resolve_attribute_type(const std::string& type_name,
                                                     const std::string& attribute,
                                                     int line) const {
    if (!type_table_.has_type(type_name)) {
        throw SemanticError(line, "tipo no definido para acceso de atributo: " + type_name);
    }

    auto inferred_it = analyzed_types_.find(type_name);
    if (inferred_it != analyzed_types_.end()) {
        const TypeInfo& inferred = inferred_it->second;
        auto attr_it = inferred.attributes.find(attribute);
        if (attr_it != inferred.attributes.end()) {
            return attr_it->second.empty() ? kObjectType : attr_it->second;
        }
    }

    const TypeInfo* current = &type_table_.get_type(type_name);
    while (current) {
        auto it = current->attributes.find(attribute);
        if (it != current->attributes.end()) {
            return it->second.empty() ? kObjectType : it->second;
        }
        if (current->parent.empty() || !type_table_.has_type(current->parent)) {
            break;
        }
        current = &type_table_.get_type(current->parent);
    }

    throw SemanticError(line, "atributo no definido: " + attribute + " en " + type_name);
}

MethodSig SemanticAnalyzer::resolve_method_sig(const std::string& type_name,
                                               const std::string& method,
                                               int line) const {
    if (!type_table_.has_type(type_name)) {
        throw SemanticError(line, "tipo no definido para llamada de método: " + type_name);
    }

    auto inferred_it = analyzed_types_.find(type_name);
    if (inferred_it != analyzed_types_.end()) {
        const TypeInfo& inferred = inferred_it->second;
        auto method_it = inferred.methods.find(method);
        if (method_it != inferred.methods.end()) {
            return method_it->second;
        }
    }

    const TypeInfo* current = &type_table_.get_type(type_name);
    while (current) {
        auto it = current->methods.find(method);
        if (it != current->methods.end()) {
            return it->second;
        }
        if (current->parent.empty() || !type_table_.has_type(current->parent)) {
            break;
        }
        current = &type_table_.get_type(current->parent);
    }

    throw SemanticError(line, "método no definido: " + method + " en " + type_name);
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
    const std::string left_type = analyze_expr(node.left.get());
    const std::string right_type = analyze_expr(node.right.get());

    if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/" || node.op == "^") {
        ensure_conforms(left_type, kNumberType, node.line, "operador aritmético");
        ensure_conforms(right_type, kNumberType, node.line, "operador aritmético");
        return kNumberType;
    }

    if (node.op == "&" || node.op == "|") {
        ensure_conforms(left_type, kBooleanType, node.line, "operador booleano");
        ensure_conforms(right_type, kBooleanType, node.line, "operador booleano");
        return kBooleanType;
    }

    if (node.op == "<" || node.op == ">" || node.op == "<=" || node.op == ">=") {
        ensure_conforms(left_type, kNumberType, node.line, "comparación numérica");
        ensure_conforms(right_type, kNumberType, node.line, "comparación numérica");
        return kBooleanType;
    }

    if (node.op == "==" || node.op == "!=") {
        ensure_equals_or_conforms(left_type, right_type, node.line, "comparación de igualdad");
        return kBooleanType;
    }

    if (node.op == "@" || node.op == "@@") {
        ensure_conforms(left_type, kStringType, node.line, "concatenación");
        ensure_conforms(right_type, kStringType, node.line, "concatenación");
        return kStringType;
    }

    return kObjectType;
}

std::string SemanticAnalyzer::visit(UnaryExpr& node) {
    const std::string operand_type = analyze_expr(node.operand.get());

    if (node.op == "-") {
        ensure_conforms(operand_type, kNumberType, node.line, "negación aritmética");
        return kNumberType;
    }
    if (node.op == "!") {
        ensure_conforms(operand_type, kBooleanType, node.line, "negación lógica");
        return kBooleanType;
    }

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
    const std::string var_type = symbols_.lookup(node.name, node.line);
    const std::string value_type = analyze_expr(node.value.get());
    ensure_conforms(value_type, var_type, node.line, "asignación");
    return var_type;
}

std::string SemanticAnalyzer::visit(LetBinding& node) {
    const std::string init_type = analyze_expr(node.initializer.get());
    std::string final_type = init_type;
    if (!node.type_annotation.empty()) {
        ensure_conforms(init_type, node.type_annotation, 0, "binding let");
        final_type = node.type_annotation;
    }
    symbols_.define(node.name, final_type, 0);
    return final_type;
}

std::string SemanticAnalyzer::visit(LetExpr& node) {
    symbols_.enter_scope();
    for (auto& binding : node.bindings) {
        visit(binding);
    }
    std::string result = analyze_expr(node.body.get());
    symbols_.exit_scope();
    return result;
}

std::string SemanticAnalyzer::visit(IfExpr& node) {
    std::string result_type = kObjectType;
    bool initialized = false;

    for (auto& branch : node.branches) {
        const std::string cond_type = analyze_expr(branch.condition.get());
        ensure_conforms(cond_type, kBooleanType, node.line, "condición if");
        const std::string body_type = analyze_expr(branch.body.get());
        if (!initialized) {
            result_type = body_type;
            initialized = true;
        } else {
            result_type = type_table_.lowest_common_ancestor(result_type, body_type);
        }
    }

    const std::string else_type = analyze_expr(node.else_body.get());
    if (!initialized) {
        result_type = else_type;
    } else {
        result_type = type_table_.lowest_common_ancestor(result_type, else_type);
    }

    return result_type.empty() ? kObjectType : result_type;
}

std::string SemanticAnalyzer::visit(WhileExpr& node) {
    const std::string cond_type = analyze_expr(node.condition.get());
    ensure_conforms(cond_type, kBooleanType, node.line, "condición while");
    const std::string body_type = analyze_expr(node.body.get());
    return body_type.empty() ? kObjectType : body_type;
}

std::string SemanticAnalyzer::visit(ForExpr& node) {
    const std::string iterable_type = analyze_expr(node.iterable.get());

    std::string element_type;
    if (TypeTable::is_iterable_type(iterable_type, &element_type)) {
        // ok
    } else if (TypeTable::is_vector_type(iterable_type, &element_type)) {
        // treat vector as iterable
    } else {
        throw SemanticError(node.line, "el for espera un iterable, se obtuvo: " + iterable_type);
    }

    symbols_.enter_scope();
    symbols_.define(node.variable_name, element_type.empty() ? kObjectType : element_type, node.line);
    const std::string body_type = analyze_expr(node.body.get());
    symbols_.exit_scope();
    return body_type.empty() ? kObjectType : body_type;
}

std::string SemanticAnalyzer::visit(FuncCall& node) {
    auto it = functions_.find(node.name);
    if (it == functions_.end()) {
        throw SemanticError(node.line, "función no definida: " + node.name);
    }

    const auto& sig = it->second;
    if (sig.param_types.size() != node.args.size()) {
        throw SemanticError(node.line, "aridad incorrecta en llamada a " + node.name);
    }

    for (std::size_t i = 0; i < node.args.size(); ++i) {
        const std::string arg_type = analyze_expr(node.args[i].get());
        const std::string param_type = sig.param_types[i].empty() ? kObjectType : sig.param_types[i];
        ensure_conforms(arg_type, param_type, node.line, "argumento de función");
    }

    return sig.return_type.empty() ? kObjectType : sig.return_type;
}

std::string SemanticAnalyzer::visit(FuncDef& node) {
    context_.enter_function(node.name);
    symbols_.enter_scope();
    for (const auto& param : node.params) {
        symbols_.define(param.name, param.type_annotation.empty() ? kObjectType : param.type_annotation, node.line);
    }
    const std::string body_type = analyze_expr(node.body.get());
    std::string result_type = body_type.empty() ? kObjectType : body_type;

    if (!node.return_type.empty()) {
        ensure_conforms(result_type, node.return_type, node.line, "retorno de función");
        result_type = node.return_type;
    }

    auto it = functions_.find(node.name);
    if (it != functions_.end()) {
        it->second.return_type = result_type;
    }

    symbols_.exit_scope();
    context_.exit_function();
    return kObjectType;
}

std::string SemanticAnalyzer::visit(TypeDef& node) {
    context_.enter_type(node.name);
    std::unordered_map<std::string, std::string> attributes;
    std::unordered_map<std::string, MethodSig> methods;

    for (const auto& attr : node.attributes) {
        if (!attr || !attr->initializer) {
            continue;
        }
        symbols_.enter_scope();
        const std::string init_type = analyze_expr(attr->initializer.get());
        symbols_.exit_scope();

        std::string final_type = init_type.empty() ? kObjectType : init_type;
        if (!attr->type_annotation.empty()) {
            ensure_conforms(final_type, attr->type_annotation, attr->line, "atributo " + attr->name);
            final_type = attr->type_annotation;
        }
        attributes[attr->name] = final_type;
    }

    const std::string parent_name = type_table_.get_type(node.name).parent;

    for (const auto& method : node.methods) {
        if (!method) {
            continue;
        }
        context_.enter_method(method->name);
        symbols_.enter_scope();

        std::vector<std::string> param_types;
        param_types.reserve(method->params.size());
        for (const auto& param : method->params) {
            const std::string param_type = param.type_annotation.empty() ? kObjectType : param.type_annotation;
            symbols_.define(param.name, param_type, method->line);
            param_types.push_back(param_type);
        }

        const std::string body_type = analyze_expr(method->body.get());
        std::string return_type = body_type.empty() ? kObjectType : body_type;
        if (!method->return_type.empty()) {
            ensure_conforms(return_type, method->return_type, method->line, "retorno de método " + method->name);
            return_type = method->return_type;
        }

        if (!parent_name.empty() && type_table_.has_type(parent_name)) {
            const TypeInfo& parent_info = type_table_.get_type(parent_name);
            auto parent_it = parent_info.methods.find(method->name);
            if (parent_it != parent_info.methods.end()) {
                const auto& parent_sig = parent_it->second;
                if (parent_sig.param_types.size() != param_types.size()) {
                    throw SemanticError(method->line, "override con distinta aridad en método " + method->name);
                }
                for (std::size_t i = 0; i < param_types.size(); ++i) {
                    const std::string parent_param = parent_sig.param_types[i].empty() ? kObjectType : parent_sig.param_types[i];
                    if (parent_param != param_types[i]) {
                        throw SemanticError(method->line, "override con firma distinta en método " + method->name);
                    }
                }
                const std::string parent_ret = parent_sig.return_type.empty() ? kObjectType : parent_sig.return_type;
                if (parent_ret != return_type) {
                    throw SemanticError(method->line, "override con tipo de retorno distinto en método " + method->name);
                }
            }
        }

        methods.emplace(method->name, MethodSig{std::move(param_types), return_type});

        symbols_.exit_scope();
        context_.exit_function();
    }

    analyzed_types_[node.name] = TypeInfo{node.name, parent_name, std::move(attributes), std::move(methods)};
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
    if (node.type_name.empty() || !type_table_.has_type(node.type_name)) {
        throw SemanticError(node.line, "tipo no definido en new: " + node.type_name);
    }

    auto ctor_it = type_constructors_.find(node.type_name);
    const std::vector<std::string> empty_ctor;
    const auto& ctor_params = ctor_it != type_constructors_.end() ? ctor_it->second : empty_ctor;

    if (ctor_params.size() != node.args.size()) {
        throw SemanticError(node.line, "aridad incorrecta en constructor de " + node.type_name);
    }

    for (std::size_t i = 0; i < node.args.size(); ++i) {
        const std::string arg_type = analyze_expr(node.args[i].get());
        const std::string param_type = ctor_params[i].empty() ? kObjectType : ctor_params[i];
        ensure_conforms(arg_type, param_type, node.line, "argumento de constructor");
    }

    return node.type_name;
}

std::string SemanticAnalyzer::visit(MemberAccess& node) {
    const std::string object_type = analyze_expr(node.object.get());
    return resolve_attribute_type(object_type, node.member_name, node.line);
}

std::string SemanticAnalyzer::visit(MethodCall& node) {
    const std::string object_type = analyze_expr(node.object.get());
    if (node.method_name == "size") {
        if (!TypeTable::is_vector_type(object_type)) {
            throw SemanticError(node.line, "size() solo es válido sobre vectores");
        }
    }
    MethodSig sig = resolve_method_sig(object_type, node.method_name, node.line);

    if (sig.param_types.size() != node.args.size()) {
        throw SemanticError(node.line, "aridad incorrecta en llamada a método " + node.method_name);
    }

    for (std::size_t i = 0; i < node.args.size(); ++i) {
        const std::string arg_type = analyze_expr(node.args[i].get());
        const std::string param_type = sig.param_types[i].empty() ? kObjectType : sig.param_types[i];
        ensure_conforms(arg_type, param_type, node.line, "argumento de método");
    }

    return sig.return_type.empty() ? kObjectType : sig.return_type;
}

std::string SemanticAnalyzer::visit(SelfRef& node) {
    if (!context_.in_method) {
        throw SemanticError(node.line, "self solo es válido dentro de métodos");
    }
    return context_.current_type.empty() ? kObjectType : context_.current_type;
}

std::string SemanticAnalyzer::visit(BaseCall& node) {
    if (!context_.in_method || context_.current_type.empty()) {
        throw SemanticError(node.line, "base() solo es válido dentro de métodos");
    }

    const TypeInfo& current_type = type_table_.get_type(context_.current_type);
    if (current_type.parent.empty()) {
        throw SemanticError(node.line, "el tipo " + context_.current_type + " no tiene padre para base()");
    }

    MethodSig sig = resolve_method_sig(current_type.parent, context_.current_function, node.line);

    if (sig.param_types.size() != node.args.size()) {
        throw SemanticError(node.line, "aridad incorrecta en llamada base() de " + context_.current_function);
    }

    for (std::size_t i = 0; i < node.args.size(); ++i) {
        const std::string arg_type = analyze_expr(node.args[i].get());
        const std::string param_type = sig.param_types[i].empty() ? kObjectType : sig.param_types[i];
        ensure_conforms(arg_type, param_type, node.line, "argumento de base()");
    }

    return sig.return_type.empty() ? kObjectType : sig.return_type;
}

std::string SemanticAnalyzer::visit(IsExpr& node) {
    analyze_expr(node.expression.get());
    if (!type_table_.has_type(node.type_name)) {
        throw SemanticError(node.line, "tipo no definido en is: " + node.type_name);
    }
    return kBooleanType;
}

std::string SemanticAnalyzer::visit(AsExpr& node) {
    const std::string expr_type = analyze_expr(node.expression.get());
    if (!type_table_.has_type(node.type_name)) {
        throw SemanticError(node.line, "tipo no definido en as: " + node.type_name);
    }

    if (!type_table_.conforms_to(expr_type, node.type_name) &&
        !type_table_.conforms_to(node.type_name, expr_type)) {
        throw SemanticError(node.line, "as inválido entre " + expr_type + " y " + node.type_name);
    }

    return node.type_name.empty() ? kObjectType : node.type_name;
}

std::string SemanticAnalyzer::visit(VectorLiteral& node) {
    if (node.elements.empty()) {
        type_table_.ensure_vector_type(kObjectType);
        return TypeTable::make_vector_type(kObjectType);
    }

    std::string element_type = analyze_expr(node.elements.front().get());
    for (std::size_t i = 1; i < node.elements.size(); ++i) {
        const std::string current = analyze_expr(node.elements[i].get());
        element_type = type_table_.lowest_common_ancestor(element_type, current);
    }

    element_type = element_type.empty() ? kObjectType : element_type;
    type_table_.ensure_vector_type(element_type);
    return TypeTable::make_vector_type(element_type);
}

std::string SemanticAnalyzer::visit(VectorComprehension& node) {
    const std::string iterable_type = analyze_expr(node.iterable.get());
    std::string element_type;

    if (TypeTable::is_iterable_type(iterable_type, &element_type)) {
        // ok
    } else if (TypeTable::is_vector_type(iterable_type, &element_type)) {
        // treat vector as iterable
    } else {
        throw SemanticError(node.line, "comprensión espera iterable, se obtuvo: " + iterable_type);
    }

    symbols_.enter_scope();
    symbols_.define(node.variable_name, element_type.empty() ? kObjectType : element_type, node.line);
    const std::string generator_type = analyze_expr(node.generator.get());
    symbols_.exit_scope();

    const std::string result_type = generator_type.empty() ? kObjectType : generator_type;
    type_table_.ensure_vector_type(result_type);
    return TypeTable::make_vector_type(result_type);
}

std::string SemanticAnalyzer::visit(VectorComprehensionFilter& node) {
    const std::string iterable_type = analyze_expr(node.iterable.get());
    std::string element_type;

    if (TypeTable::is_iterable_type(iterable_type, &element_type)) {
        // ok
    } else if (TypeTable::is_vector_type(iterable_type, &element_type)) {
        // treat vector as iterable
    } else {
        throw SemanticError(node.line, "comprensión espera iterable, se obtuvo: " + iterable_type);
    }

    symbols_.enter_scope();
    symbols_.define(node.variable_name, element_type.empty() ? kObjectType : element_type, node.line);
    const std::string generator_type = analyze_expr(node.generator.get());
    const std::string filter_type = analyze_expr(node.filter.get());
    symbols_.exit_scope();

    ensure_conforms(filter_type, kBooleanType, node.line, "filtro de comprensión");

    const std::string result_type = generator_type.empty() ? kObjectType : generator_type;
    type_table_.ensure_vector_type(result_type);
    return TypeTable::make_vector_type(result_type);
}

std::string SemanticAnalyzer::visit(VectorIndex& node) {
    const std::string vector_type = analyze_expr(node.vector.get());
    const std::string index_type = analyze_expr(node.index.get());
    ensure_conforms(index_type, kNumberType, node.line, "índice de vector");

    std::string element_type;
    if (!TypeTable::is_vector_type(vector_type, &element_type)) {
        throw SemanticError(node.line, "indexado requiere vector, se obtuvo: " + vector_type);
    }

    return element_type.empty() ? kObjectType : element_type;
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
