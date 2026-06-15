#ifndef SERIALIZE_HPP
#define SERIALIZE_HPP

#include <string>

#include "bytecode.hpp"

// Binary format version for forward compatibility.
constexpr uint32_t kSerializeMagic = 0x48554C4B; // "HULK"
constexpr uint32_t kSerializeVersion = 1;

// Serialize a BytecodeProgram to a binary file at the given path.
// The format is: magic | version | code_count | instructions... |
//                constants_count | constants... | function_table_count | entries...
void serialize(const BytecodeProgram& prog, const std::string& path);

// Deserialize a BytecodeProgram from a binary file at the given path.
// Throws std::runtime_error if the file cannot be opened or is corrupt.
BytecodeProgram deserialize(const std::string& path);

#endif