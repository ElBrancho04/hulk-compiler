#include <iostream>
#include "ast.hpp"

extern int yyparse();
extern Node* root;

int main() {
    std::cout << "--- HULK Compiler (Parser Mode) ---" << std::endl;
    if (yyparse() == 0) {
        std::cout << "Análisis sintáctico completado con éxito." << std::endl;
    } else {
        std::cout << "Análisis fallido." << std::endl;
    }
    return 0;
}