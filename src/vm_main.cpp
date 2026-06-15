#include <cstdlib>
#include <iostream>
#include <string>

#include "serialize.hpp"
#include "vm.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: hulk-vm <compiled.asm>\n";
        return 1;
    }

    try {
        BytecodeProgram program = deserialize(argv[1]);
        VM vm;
        vm.execute(program);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}