#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "ast.hpp"
#include "code_generator.hpp"
#include "semantic_analyzer.hpp"
#include "serialize.hpp"
#include "vm.hpp"

extern Program* root;
extern FILE* yyin;
extern int yyparse();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: hulk <archivo.hulk>\n";
        return 1;
    }

    // Open source file for the lexer.
    FILE* source = std::fopen(argv[1], "r");
    if (!source) {
        std::cerr << "Error: no se pudo abrir el archivo '" << argv[1] << "'\n";
        return 1;
    }
    yyin = source;

    // Parse.
    if (yyparse() != 0 || root == nullptr) {
        std::cerr << "Error de sintaxis.\n";
        std::fclose(source);
        return 2;
    }
    std::fclose(source);

    // Semantic analysis.
    try {
        SemanticAnalyzer analyzer;
        analyzer.analyze(*root);
    } catch (const SemanticError& e) {
        std::cerr << e.what() << "\n";
        delete root;
        return 3;
    }

    // Code generation.
    BytecodeProgram program;
    CodeGenerator codegen(program);
    root->accept(codegen);
    delete root;
    root = nullptr;

    // Serialize bytecode to compiled.asm.
    try {
        serialize(program, "compiled.asm");
    } catch (const std::exception& e) {
        std::cerr << "Error al serializar bytecode: " << e.what() << "\n";
        return 1;
    }

    // Generate executable script ./output.
    {
        std::ofstream out("./output");
        if (!out.is_open()) {
            std::cerr << "Error: no se pudo crear './output'\n";
            return 1;
        }
        out << "#!/bin/bash\n";
        out << "./hulk-vm compiled.asm\n";
    }

    // Make ./output executable.
    if (std::system("chmod +x ./output") != 0) {
        std::cerr << "Error: no se pudo hacer ejecutable './output'\n";
        return 1;
    }

    return 0;
}