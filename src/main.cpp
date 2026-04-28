#include <iostream>
#include <string>

// Declaraciones externas de Flex
extern int yylex();
extern char* yytext;
extern int line_number;
extern std::string parsed_string; // Importamos la variable del lexer

int main() {
    std::cout << "--- HULK Lexer Test Mode ---" << std::endl;
    std::cout << "Escribe tu codigo y pulsa Enter. (Ctrl+D para salir)" << std::endl;

    int token;
    while ((token = yylex()) != 0) {
        if (token >= 100) {
            std::cout << "KEYWORD/OP (ID " << token << "): [" << yytext << "]" << std::endl;
        } else if (token == 4) {
            std::cout << "IDENTIFIER: [" << yytext << "]" << std::endl;
        } else if (token == 2) {
            std::cout << "NUMBER: [" << yytext << "]" << std::endl;
        } else if (token == 3) {
            std::cout << "STRING COMPLETE: [" << parsed_string << "]" << std::endl;
        } else if (token > 0 && token <= 255) {
            std::cout << "SIMPLE CHAR: [" << (char)token << "]" << std::endl;
        }
    }

    std::cout << "\n--- Test Finalizado ---" << std::endl;
    return 0;
}