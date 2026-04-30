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

# Ejecutable principal
add_executable(hulk
    src/main.cpp
    ${FLEX_MyLexer_OUTPUTS}
    ${BISON_MyParser_OUTPUTS}
)

add_executable(runtime_test
    tests/runtime_test.cpp
)

add_executable(bytecode_smoke_test
    tests/bytecode_smoke_test.cpp
)