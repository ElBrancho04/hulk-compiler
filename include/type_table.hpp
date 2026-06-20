#ifndef TYPE_TABLE_HPP
#define TYPE_TABLE_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct MethodSig {
    std::vector<std::string> param_types;
    std::string return_type;

    MethodSig() = default;
    MethodSig(std::vector<std::string> params, std::string ret)
        : param_types(std::move(params)), return_type(std::move(ret)) {}
};

struct TypeInfo {
    std::string name;
    std::string parent; // empty means no parent
    std::unordered_map<std::string, std::string> attributes;
    std::unordered_map<std::string, MethodSig> methods;

    TypeInfo() = default;
    TypeInfo(std::string name,
             std::string parent,
             std::unordered_map<std::string, std::string> attributes = {},
             std::unordered_map<std::string, MethodSig> methods = {})
        : name(std::move(name)),
          parent(std::move(parent)),
          attributes(std::move(attributes)),
          methods(std::move(methods)) {}
};

struct ProtocolInfo {
    std::string name;
    std::string parent;
    std::unordered_map<std::string, MethodSig> methods;

    ProtocolInfo() = default;
    ProtocolInfo(std::string name,
                 std::string parent,
                 std::unordered_map<std::string, MethodSig> methods = {})
        : name(std::move(name)),
          parent(std::move(parent)),
          methods(std::move(methods)) {}
};

class TypeTable {
public:
    TypeTable();

    void register_type(TypeInfo type);
    bool has_type(const std::string& name) const;
    const TypeInfo& get_type(const std::string& name) const;

    void register_protocol(ProtocolInfo protocol);
    bool has_protocol(const std::string& name) const;
    const ProtocolInfo& get_protocol(const std::string& name) const;

    bool conforms_to(const std::string& derived, const std::string& target) const;
    std::string lowest_common_ancestor(const std::string& a, const std::string& b) const;

    void ensure_vector_type(const std::string& element_type);
    void ensure_iterable_type(const std::string& element_type);

    static std::string make_vector_type(const std::string& element_type);
    static std::string make_iterable_type(const std::string& element_type);
    static bool is_vector_type(const std::string& type_name, std::string* element_out = nullptr);
    static bool is_iterable_type(const std::string& type_name, std::string* element_out = nullptr);

    // Functor types: (T1,T2)->R  ->  _Functor_N with invoke(T1,T2): R
    void ensure_functor_type(const std::vector<std::string>& param_types,
                             const std::string& return_type);
    static bool is_functor_type_name(const std::string& type_name,
                                     std::vector<std::string>* param_types_out = nullptr,
                                     std::string* return_type_out = nullptr);
    static std::string make_functor_type_name(const std::vector<std::string>& param_types,
                                              const std::string& return_type);
    // Parse functional type annotation string like "_FuncType(T1,T2)->R"
    static bool parse_functional_annotation(const std::string& annotation,
                                            std::vector<std::string>* param_types_out,
                                            std::string* return_type_out);

private:
    std::unordered_map<std::string, TypeInfo> types_;
    std::unordered_set<std::string> final_types_;
    std::unordered_set<std::string> synthetic_types_;
    std::unordered_map<std::string, ProtocolInfo> protocols_;

    void register_builtin_types();
    static bool parse_container_type(const std::string& type_name,
                                     const std::string& prefix,
                                     std::string* element_out);
    bool is_synthetic_type(const std::string& type_name) const;
    void ensure_element_type(const std::string& element_type);
    bool type_has_methods_for_protocol(const std::string& type_name,
                                      const ProtocolInfo& protocol) const;
    static bool conforms_with_variance(const MethodSig& derived,
                                       const MethodSig& protocol_sig,
                                       const TypeTable& table);
    std::unordered_map<std::string, MethodSig> get_all_protocol_methods(const std::string& protocol_name) const;
};

#endif
