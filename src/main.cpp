#include <iostream>
#include "ast.hpp"

extern int yylex();
extern int line_number;

int main() {
    std::cout << "HULK Compiler - Testing Lexer Compilation" << std::endl;
    return 0;
}