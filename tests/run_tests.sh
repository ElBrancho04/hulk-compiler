#!/bin/bash
# Compilar si no existe el binario
if [ ! -f ./build/hulk ]; then
    echo "Error: Binario no encontrado en build/hulk"
    exit 1
fi

# Por ahora un test simple que verifica si el compilador abre
./build/hulk > /dev/null
if [ $? -eq 0 ]; then
    echo "Test inicial: PASSED"
    exit 0
else
    echo "Test inicial: FAILED"
    exit 1
fi