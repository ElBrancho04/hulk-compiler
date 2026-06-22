#include "type_table.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>

namespace {
constexpr const char* kObjectType = "Object";
constexpr const char* kNumberType = "Number";
constexpr const char* kStringType = "String";
constexpr const char* kBooleanType = "Boolean";

constexpr const char* kVectorPrefix = "Vector<";
constexpr const char* kIterablePrefix = "Iterable<";
constexpr const char* kContainerSuffix = ">";

constexpr const char* kFunctorTypePrefix = "_FunctorType_";
constexpr const char* kFuncAnnotationPrefix = "_FuncType(";
constexpr const char* kFuncAnnotationSuffix = ")";
constexpr const char* kFuncAnnotationSep = ",";
constexpr const char* kFuncRetSep = "->";

constexpr const char* kInvokeMethod = "invoke";
} // namespace

TypeTable::TypeTable() {
    register_builtin_types();
}

void TypeTable::register_builtin_types() {
    final_types_.insert(kNumberType);
    final_types_.insert(kStringType);
    final_types_.insert(kBooleanType);

    types_.emplace(kObjectType, TypeInfo{kObjectType, ""});
    types_.emplace(kNumberType, TypeInfo{kNumberType, kObjectType});
    types_.emplace(kStringType, TypeInfo{kStringType, kObjectType});
    types_.emplace(kBooleanType, TypeInfo{kBooleanType, kObjectType});

    // Register base Iterable protocol: next():Boolean, current():Object
    ProtocolInfo iterable;
    iterable.name = "Iterable";
    iterable.parent = "";
    iterable.methods.emplace("next", MethodSig({}, kBooleanType));
    iterable.methods.emplace("current", MethodSig({}, kObjectType));
    protocols_.emplace("Iterable", std::move(iterable));
}

void TypeTable::register_protocol(ProtocolInfo protocol) {
    if (protocol.name.empty()) {
        throw std::runtime_error("TypeTable: el nombre del protocolo no puede ser vacío.");
    }
    if (protocols_.count(protocol.name) > 0) {
        throw std::runtime_error("TypeTable: protocolo duplicado: " + protocol.name);
    }
    protocols_.emplace(protocol.name, std::move(protocol));
}

bool TypeTable::has_protocol(const std::string& name) const {
    return protocols_.count(name) > 0;
}

const ProtocolInfo& TypeTable::get_protocol(const std::string& name) const {
    auto it = protocols_.find(name);
    if (it == protocols_.end()) {
        throw std::runtime_error("TypeTable: protocolo inexistente: " + name);
    }
    return it->second;
}

void TypeTable::register_type(TypeInfo type) {
    if (type.name.empty()) {
        throw std::runtime_error("TypeTable: el nombre del tipo no puede ser vacío.");
    }
    if (types_.count(type.name) > 0) {
        throw std::runtime_error("TypeTable: tipo duplicado: " + type.name);
    }

    if (type.parent.empty()) {
        type.parent = kObjectType;
    }

    if (final_types_.count(type.parent) > 0) {
        throw std::runtime_error("TypeTable: no se puede heredar de " + type.parent);
    }

    if (type.name != kObjectType && types_.count(type.parent) == 0) {
        throw std::runtime_error("TypeTable: tipo padre inexistente: " + type.parent);
    }

    types_.emplace(type.name, std::move(type));
}

bool TypeTable::has_type(const std::string& name) const {
    return types_.count(name) > 0;
}

const TypeInfo& TypeTable::get_type(const std::string& name) const {
    auto it = types_.find(name);
    if (it == types_.end()) {
        throw std::runtime_error("TypeTable: tipo inexistente: " + name);
    }
    return it->second;
}

bool TypeTable::conforms_to(const std::string& derived, const std::string& target) const {
    if (derived == target) {
        return true;
    }
    if (target == kObjectType) {
        return true;
    }

    std::string derived_element;
    std::string target_element;

    // Handle synthetic container types (Vector<T>, Iterable<T>) first,
    // before protocol checking, since Iterable<T> is both a container type
    // and a protocol.
    if (is_vector_type(derived, &derived_element)) {
        if (is_vector_type(target, &target_element))   return conforms_to(derived_element, target_element);
        if (is_iterable_type(target, &target_element)) return conforms_to(derived_element, target_element);
        if (has_protocol(target)) {
            // Let it fall through to protocol checking!
        } else {
            return false;
        }
    }
    if (is_iterable_type(derived, &derived_element)) {
        if (is_iterable_type(target, &target_element)) return conforms_to(derived_element, target_element);
        if (has_protocol(target)) {
            // Let it fall through to protocol checking!
        } else {
            return false;
        }
    }
    if ((is_vector_type(target) || is_iterable_type(target)) && !has_protocol(target)) {
        return false;
    }

    // --- Target es un protocolo: conformidad estructural ---
    if (has_protocol(target)) {
        const ProtocolInfo& proto = get_protocol(target);
        // derived puede ser un tipo nominal o un protocolo
        if (has_type(derived)) {
            return type_has_methods_for_protocol(derived, proto);
        }
        if (has_protocol(derived)) {
            // Si P extends Q (directa o transitivamente), entonces P <= Q
            const ProtocolInfo& derived_proto = get_protocol(derived);
            const std::string* current_parent = &derived_proto.parent;
            while (!current_parent->empty()) {
                if (*current_parent == target) {
                    return true;
                }
                if (!has_protocol(*current_parent)) {
                    break;
                }
                current_parent = &get_protocol(*current_parent).parent;
            }
            // Verificación estructural con variancia: que derived tenga todos los métodos de target (incluyendo heredados)
            auto target_methods = get_all_protocol_methods(target);
            auto derived_methods = get_all_protocol_methods(derived);
            for (const auto& [mname, msig] : target_methods) {
                auto it = derived_methods.find(mname);
                if (it == derived_methods.end()) {
                    return false;
                }
                if (!conforms_with_variance(it->second, msig, *this)) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    if (!has_type(derived) || !has_type(target)) {
        throw std::runtime_error("TypeTable: conformidad con tipo inexistente.");
    }

    const TypeInfo* current = &get_type(derived);
    while (!current->parent.empty()) {
        if (current->parent == target) {
            return true;
        }
        if (!has_type(current->parent)) {
            break;
        }
        current = &get_type(current->parent);
    }
    return false;
}

bool TypeTable::type_has_methods_for_protocol(const std::string& type_name,
                                              const ProtocolInfo& protocol) const {
    // Recorrer la jerarquía completa del tipo, incluyendo métodos heredados de todos los protocolos base
    auto all_proto_methods = get_all_protocol_methods(protocol.name);
    for (const auto& [mname, msig] : all_proto_methods) {
        bool found = false;
        const TypeInfo* current = &get_type(type_name);
        do {
            auto it = current->methods.find(mname);
            if (it != current->methods.end()) {
                // Verificar con variancia
                if (conforms_with_variance(it->second, msig, *this)) {
                    found = true;
                    break;
                }
            }
            if (current->parent.empty() || !has_type(current->parent)) {
                break;
            }
            current = &get_type(current->parent);
        } while (true);

        if (!found) {
            return false;
        }
    }
    return true;
}

bool TypeTable::conforms_with_variance(const MethodSig& derived,
                                       const MethodSig& protocol_sig,
                                       const TypeTable& table) {
    // Args contravariantes: protocol.param <= derived.param
    if (derived.param_types.size() != protocol_sig.param_types.size()) {
        return false;
    }
    for (std::size_t i = 0; i < derived.param_types.size(); ++i) {
        const std::string& p = protocol_sig.param_types[i];
        const std::string& d = derived.param_types[i];
        if (p != d && !table.conforms_to(p, d)) {
            return false;
        }
    }
    // Return covariante: derived.return <= protocol.return
    const std::string& derived_ret = derived.return_type.empty() ? kObjectType : derived.return_type;
    const std::string& protocol_ret = protocol_sig.return_type.empty() ? kObjectType : protocol_sig.return_type;
    if (derived_ret != protocol_ret && !table.conforms_to(derived_ret, protocol_ret)) {
        return false;
    }
    return true;
}

std::string TypeTable::lowest_common_ancestor(const std::string& a, const std::string& b) const {
    if (a == b) {
        return a;
    }

    std::string a_element;
    std::string b_element;

    if (is_vector_type(a, &a_element) && is_vector_type(b, &b_element)) {
        return make_vector_type(lowest_common_ancestor(a_element, b_element));
    }
    if (is_iterable_type(a, &a_element) && is_iterable_type(b, &b_element)) {
        return make_iterable_type(lowest_common_ancestor(a_element, b_element));
    }
    if (is_vector_type(a) || is_iterable_type(a) || is_vector_type(b) || is_iterable_type(b)) {
        return kObjectType;
    }

    if (!has_type(a) || !has_type(b)) {
        throw std::runtime_error("TypeTable: LCA con tipo inexistente.");
    }

    std::unordered_set<std::string> ancestors;
    const TypeInfo* current = &get_type(a);
    ancestors.insert(current->name);
    while (!current->parent.empty()) {
        ancestors.insert(current->parent);
        if (!has_type(current->parent)) {
            break;
        }
        current = &get_type(current->parent);
    }

    current = &get_type(b);
    if (ancestors.count(current->name) > 0) {
        return current->name;
    }
    while (!current->parent.empty()) {
        if (ancestors.count(current->parent) > 0) {
            return current->parent;
        }
        if (!has_type(current->parent)) {
            break;
        }
        current = &get_type(current->parent);
    }

    return kObjectType;
}

void TypeTable::ensure_vector_type(const std::string& element_type) {
    const std::string type_name = make_vector_type(element_type);
    if (types_.count(type_name) > 0) {
        return;
    }

    ensure_element_type(element_type);

    TypeInfo info{type_name, kObjectType};
    info.methods.emplace("size", MethodSig({}, kNumberType));
    info.methods.emplace("iter", MethodSig({}, make_iterable_type(element_type)));
    info.methods.emplace("next", MethodSig({}, kBooleanType));
    info.methods.emplace("current", MethodSig({}, element_type));
    register_type(std::move(info));
    synthetic_types_.insert(type_name);
}

void TypeTable::ensure_iterable_type(const std::string& element_type) {
    const std::string type_name = make_iterable_type(element_type);
    if (types_.count(type_name) > 0) {
        return;
    }

    ensure_element_type(element_type);

    // Register Iterable<T> as a protocol extending Iterable: current(): T
    if (!has_protocol(type_name)) {
        ProtocolInfo proto;
        proto.name = type_name;
        proto.parent = "Iterable";
        proto.methods.emplace("next", MethodSig({}, kBooleanType));
        proto.methods.emplace("current", MethodSig({}, element_type));
        // Use direct map insertion to avoid duplicate check with register_protocol
        // since the protocol name follows the Iterable<T> convention
        protocols_.emplace(type_name, std::move(proto));
    }

    // Also register as TypeInfo for method resolution (next, current)
    TypeInfo info{type_name, kObjectType};
    info.methods.emplace("next", MethodSig({}, kBooleanType));
    info.methods.emplace("current", MethodSig({}, element_type));
    register_type(std::move(info));
    synthetic_types_.insert(type_name);
}

std::string TypeTable::make_vector_type(const std::string& element_type) {
    return std::string(kVectorPrefix) + element_type + kContainerSuffix;
}

std::string TypeTable::make_iterable_type(const std::string& element_type) {
    return std::string(kIterablePrefix) + element_type + kContainerSuffix;
}

bool TypeTable::is_vector_type(const std::string& type_name, std::string* element_out) {
    return parse_container_type(type_name, kVectorPrefix, element_out);
}

bool TypeTable::is_iterable_type(const std::string& type_name, std::string* element_out) {
    return parse_container_type(type_name, kIterablePrefix, element_out);
}

bool TypeTable::parse_container_type(const std::string& type_name,
                                     const std::string& prefix,
                                     std::string* element_out) {
    if (type_name.rfind(prefix, 0) != 0) {
        return false;
    }
    if (type_name.size() <= prefix.size() + 1) {
        return false;
    }
    if (type_name.back() != kContainerSuffix[0]) {
        return false;
    }
    if (element_out) {
        *element_out = type_name.substr(prefix.size(), type_name.size() - prefix.size() - 1);
    }
    return true;
}

bool TypeTable::is_synthetic_type(const std::string& type_name) const {
    return is_vector_type(type_name) || is_iterable_type(type_name);
}

void TypeTable::ensure_element_type(const std::string& element_type) {
    std::string inner;
    if (is_vector_type(element_type, &inner)) {
        ensure_vector_type(inner);
        return;
    }
    if (is_iterable_type(element_type, &inner)) {
        ensure_iterable_type(inner);
        return;
    }
    if (!has_type(element_type)) {
        throw std::runtime_error("TypeTable: tipo elemento inexistente: " + element_type);
    }
}

std::unordered_map<std::string, MethodSig> TypeTable::get_all_protocol_methods(const std::string& protocol_name) const {
    std::unordered_map<std::string, MethodSig> all_methods;
    std::string current = protocol_name;
    while (!current.empty() && has_protocol(current)) {
        const ProtocolInfo& proto = get_protocol(current);
        for (const auto& [mname, msig] : proto.methods) {
            all_methods.emplace(mname, msig);
        }
        current = proto.parent;
    }
    return all_methods;
}

// ─── Functor Types ─────────────────────────────────────────────────────────

void TypeTable::ensure_functor_type(const std::vector<std::string>& param_types,
                                    const std::string& return_type) {
    const std::string type_name = make_functor_type_name(param_types, return_type);
    if (types_.count(type_name) > 0) {
        return;  // Already registered
    }

    // Build invoke signature
    std::vector<std::string> invoke_param_types = param_types;
    std::string invoke_return = return_type.empty() ? kObjectType : return_type;

    // Register as a ProtocolInfo (so structural conformance works via type_has_methods_for_protocol)
    if (!has_protocol(type_name)) {
        ProtocolInfo proto;
        proto.name = type_name;
        proto.parent = "";
        proto.methods.emplace(kInvokeMethod, MethodSig{invoke_param_types, invoke_return});
        protocols_.emplace(type_name, std::move(proto));
    }

    // Also register as TypeInfo for NEW, method resolution, etc.
    TypeInfo info{type_name, kObjectType};
    info.methods.emplace(kInvokeMethod, MethodSig{std::move(invoke_param_types), invoke_return});
    register_type(std::move(info));
    synthetic_types_.insert(type_name);
}

bool TypeTable::is_functor_type_name(const std::string& type_name,
                                     std::vector<std::string>* param_types_out,
                                     std::string* return_type_out) {
    // Pattern: _FunctorType_N where N is an integer
    const std::string& prefix = kFunctorTypePrefix;
    if (type_name.rfind(prefix, 0) != 0) {
        return false;
    }
    // The rest should be a number (we don't need to decode it, just confirm pattern)
    std::string num_part = type_name.substr(prefix.size());
    if (num_part.empty()) {
        return false;
    }
    for (char c : num_part) {
        if (!std::isdigit(c)) {
            return false;
        }
    }

    // If caller wants the actual param types and return type, they need to look them up
    // via the TypeInfo in TypeTable
    if (param_types_out || return_type_out) {
        // This can only be filled by looking up the already-registered type
        // So we leave empty here; caller should use get_type() if needed
    }
    return true;
}

std::string TypeTable::make_functor_type_name(const std::vector<std::string>& param_types,
                                              const std::string& return_type) {
    // Build a deterministic name from param/return types so we can deduplicate
    // Format: _FunctorType_<hash-or-counter>
    // We use a static counter for simplicity, keyed by the string representation
    static std::unordered_map<std::string, int> functor_name_cache;
    static int functor_counter = 0;

    // Build a signature key
    std::string key;
    for (const auto& pt : param_types) {
        if (!key.empty()) key += kFuncAnnotationSep;
        key += pt;
    }
    key += kFuncRetSep;
    key += return_type.empty() ? kObjectType : return_type;

    auto it = functor_name_cache.find(key);
    if (it != functor_name_cache.end()) {
        return kFunctorTypePrefix + std::to_string(it->second);
    }

    int id = functor_counter++;
    functor_name_cache[key] = id;
    return kFunctorTypePrefix + std::to_string(id);
}

bool TypeTable::parse_functional_annotation(const std::string& annotation,
                                            std::vector<std::string>* param_types_out,
                                            std::string* return_type_out) {
    // Format: _FuncType(T1,T2)->R
    const std::string& prefix = kFuncAnnotationPrefix;
    if (annotation.rfind(prefix, 0) != 0) {
        return false;
    }

    // Find the closing paren: first ')' after the prefix
    std::size_t paren_pos = annotation.find(')', prefix.size());
    if (paren_pos == std::string::npos) {
        return false;
    }

    // Extract params substring
    std::string params_str = annotation.substr(prefix.size(), paren_pos - prefix.size());

    // Find -> separator
    std::string suffix = annotation.substr(paren_pos + 1);
    if (suffix.rfind(kFuncRetSep, 0) != 0) {
        return false;
    }
    std::string ret_type = suffix.substr(strlen(kFuncRetSep));

    if (param_types_out) {
        param_types_out->clear();
        if (!params_str.empty()) {
            std::size_t start = 0;
            while (start < params_str.size()) {
                std::size_t comma = params_str.find(kFuncAnnotationSep, start);
                if (comma == std::string::npos) {
                    param_types_out->push_back(params_str.substr(start));
                    break;
                }
                param_types_out->push_back(params_str.substr(start, comma - start));
                start = comma + 1;
            }
        }
    }

    if (return_type_out) {
        *return_type_out = ret_type;
    }

    return true;
}
