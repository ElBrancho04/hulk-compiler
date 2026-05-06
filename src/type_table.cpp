#include "type_table.hpp"

#include <stdexcept>

namespace {
constexpr const char* kObjectType = "Object";
constexpr const char* kNumberType = "Number";
constexpr const char* kStringType = "String";
constexpr const char* kBooleanType = "Boolean";

constexpr const char* kVectorPrefix = "Vector<";
constexpr const char* kIterablePrefix = "Iterable<";
constexpr const char* kContainerSuffix = ">";
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

    if (is_vector_type(derived, &derived_element) && is_vector_type(target, &target_element)) {
        return conforms_to(derived_element, target_element);
    }
    if (is_iterable_type(derived, &derived_element) && is_iterable_type(target, &target_element)) {
        return conforms_to(derived_element, target_element);
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
    register_type(std::move(info));
    synthetic_types_.insert(type_name);
}

void TypeTable::ensure_iterable_type(const std::string& element_type) {
    const std::string type_name = make_iterable_type(element_type);
    if (types_.count(type_name) > 0) {
        return;
    }

    ensure_element_type(element_type);

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
