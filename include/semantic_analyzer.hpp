#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "ast.hpp"
#include "semantic_context.hpp"
#include "semantic_error.hpp"
#include "symbol_table.hpp"
#include "type_table.hpp"

struct FunctionSig {
    std::vector<std::string> param_types;
    std::string return_type;
};

struct ProtocolInfo {
    std::string name;
    std::string parent;
    std::unordered_map<std::string, MethodSig> methods;
};

class SemanticAnalyzer : public Visitor<std::string> {
public:
    SemanticAnalyzer();

    void analyze(Program& program);

    // Access collected non-fatal errors after analysis
    const std::vector<SemanticError>& errors() const { return errors_; }

    std::string visit(NumberLiteral& node) override;
    std::string visit(StringLiteral& node) override;
    std::string visit(BoolLiteral& node) override;
    std::string visit(BinaryExpr& node) override;
    std::string visit(UnaryExpr& node) override;
    std::string visit(BlockExpr& node) override;
    std::string visit(VarRef& node) override;
    std::string visit(AssignExpr& node) override;
    std::string visit(LetBinding& node) override;
    std::string visit(LetExpr& node) override;
    std::string visit(IfExpr& node) override;
    std::string visit(WhileExpr& node) override;
    std::string visit(ForExpr& node) override;
    std::string visit(FuncCall& node) override;
    std::string visit(FuncDef& node) override;
    std::string visit(TypeDef& node) override;
    std::string visit(ProtocolDef& node) override;
    std::string visit(ProtocolMethodSig& node) override;
    std::string visit(NewExpr& node) override;
    std::string visit(MemberAccess& node) override;
    std::string visit(MethodCall& node) override;
    std::string visit(SelfRef& node) override;
    std::string visit(BaseCall& node) override;
    std::string visit(IsExpr& node) override;
    std::string visit(AsExpr& node) override;
    std::string visit(VectorLiteral& node) override;
    std::string visit(VectorComprehension& node) override;
    std::string visit(VectorComprehensionFilter& node) override;
    std::string visit(VectorIndex& node) override;
    std::string visit(Program& node) override;
    std::string visit(LambdaExpr& node) override;

private:
    TypeTable type_table_;
    SymbolTable symbols_;
    SemanticContext context_;
    std::unordered_map<std::string, FunctionSig> functions_;
    std::unordered_map<std::string, ProtocolInfo> protocols_;
    std::unordered_map<std::string, std::vector<std::string>> type_constructors_;
    std::unordered_map<std::string, TypeInfo> analyzed_types_;
    int lambda_counter_ = 0;
    Program* current_program_ = nullptr;
    std::vector<SemanticError> errors_;

    void pass1_register_types(Program& program);
    void pass2_register_functions(Program& program);
    void pass3_type_check(Program& program);
    std::string analyze_expr(Expr* expr);
    void register_builtins();
    void ensure_type_registered(const std::string& type_name, int line);
    std::string resolve_attribute_type(const std::string& type_name, const std::string& attribute, int line);
    MethodSig resolve_method_sig(const std::string& type_name, const std::string& method, int line);
    void ensure_conforms(const std::string& actual,
                         const std::string& expected,
                         int line,
                         const std::string& context);
    void ensure_equals_or_conforms(const std::string& left,
                                   const std::string& right,
                                   int line,
                                   const std::string& context);
    void report_error(int line, int col, const std::string& message);

    void register_protocols(const std::vector<std::unique_ptr<ProtocolDef>>& protocols);
    static void validate_no_cycles(const std::unordered_map<std::string, std::string>& parents,
                                   const std::unordered_map<std::string, int>& lines);
    static void validate_no_duplicates(const std::vector<Parameter>& params, int line, const std::string& owner);
};

#endif
