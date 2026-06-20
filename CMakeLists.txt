cmake_minimum_required(VERSION 3.10)
project(HulkCompiler VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar paquetes de Flex y Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Directorios de inclusión
include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

# Configuración de Flex y Bison (puntos 4 y 5)
# Por ahora los definimos aunque no tengamos los archivos .l y .y aún
# Esto permitirá que CMake sepa cómo manejarlos después
FLEX_TARGET(MyLexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(MyParser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyLexer MyParser)

# Ejecutable principal (compiler)
add_executable(hulk
    src/main.cpp
    src/code_generator.cpp
    src/macro_expander.cpp
    src/semantic_analyzer.cpp
    src/semantic_utils.cpp
    src/serialize.cpp
    src/vm.cpp
    src/type_table.cpp
    ${FLEX_MyLexer_OUTPUTS}
    ${BISON_MyParser_OUTPUTS}
)

# Ejecutable standalone de la VM (hulk-vm)
add_executable(hulk-vm
    src/vm_main.cpp
    src/serialize.cpp
    src/vm.cpp
)

add_executable(runtime_test
    tests/runtime_test.cpp
)

add_executable(bytecode_smoke_test
    tests/bytecode_smoke_test.cpp
)

add_executable(codegen_smoke_test
    tests/codegen_smoke_test.cpp
    src/code_generator.cpp
)

add_executable(vm_smoke_test
    tests/vm_smoke_test.cpp
    src/vm.cpp
)

add_executable(for_loop_e2e_test
    tests/for_loop_e2e_test.cpp
    src/code_generator.cpp
    src/vm.cpp
)

add_executable(type_table_smoke_test
    tests/type_table_smoke_test.cpp
    src/type_table.cpp
)

add_executable(semantic_utils_smoke_test
    tests/semantic_utils_smoke_test.cpp
)

add_executable(semantic_analyzer_smoke_test
    tests/semantic_analyzer_smoke_test.cpp
    src/semantic_analyzer.cpp
    src/semantic_utils.cpp
    src/type_table.cpp
)

add_executable(lambda_smoke_test
    tests/lambda_smoke_test.cpp
    src/semantic_analyzer.cpp
    src/semantic_utils.cpp
    src/type_table.cpp
)

add_executable(type_annotations_smoke_test
    tests/type_annotations_smoke_test.cpp
    src/semantic_analyzer.cpp
    src/semantic_utils.cpp
    src/type_table.cpp
)

add_executable(functor_smoke_test
    tests/functor_smoke_test.cpp
    src/semantic_analyzer.cpp
    src/semantic_utils.cpp
    src/type_table.cpp
    src/code_generator.cpp
    src/vm.cpp
)

add_executable(macro_expander_test
    tests/macro_expander_test.cpp
    src/macro_expander.cpp
)
