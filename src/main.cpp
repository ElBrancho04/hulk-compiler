#include <iostream>
#include "ast.hpp"
#include "print_visitor.hpp"

extern Program* root;
extern int yyparse();

int main() {
    if (yyparse() == 0 && root != nullptr) {
        std::cout << "SUCCESS: Parseo completado y AST generado.\n" << std::endl;

        PrintVisitor printer;
        root->accept(printer);

        delete root; 
        return 0;
    } else {
        std::cerr << "FAILURE: Error en el parseo." << std::endl;
        return 1;
    }
}