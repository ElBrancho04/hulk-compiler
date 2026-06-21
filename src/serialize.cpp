#include "serialize.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "value.hpp"

namespace {

// Write a primitive value to an output stream.
template <typename T>
void write_val(std::ostream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

// Read a primitive value from an input stream.
template <typename T>
T read_val(std::istream& in) {
    T v;
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

// Write a string: length (uint32_t) + raw bytes (no null terminator).
void write_string(std::ostream& out, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    write_val(out, len);
    if (len > 0) {
        out.write(s.data(), len);
    }
}

// Read a string from the stream.
std::string read_string(std::istream& in) {
    uint32_t len = read_val<uint32_t>(in);
    if (len == 0) return "";
    std::string s(len, '\0');
    in.read(&s[0], len);
    return s;
}

void write_instruction(std::ostream& out, const Instruction& inst) {
    int32_t opcode = static_cast<int32_t>(inst.opcode);
    write_val(out, opcode);
    write_val(out, inst.index);
    write_val(out, inst.offset);
    write_val(out, inst.count);
    write_string(out, inst.name);
}

Instruction read_instruction(std::istream& in) {
    int32_t opcode = read_val<int32_t>(in);
    int32_t index = read_val<int32_t>(in);
    int32_t offset = read_val<int32_t>(in);
    int32_t count = read_val<int32_t>(in);
    std::string name = read_string(in);

    Instruction inst(static_cast<OpCode>(opcode));
    inst.index = index;
    inst.offset = offset;
    inst.count = count;
    inst.name = std::move(name);
    return inst;
}

void write_constant(std::ostream& out, const Value& val) {
    int32_t type_id = static_cast<int32_t>(val.type);
    write_val(out, type_id);

    switch (val.type) {
        case ValueType::Number:
            write_val(out, val.number_value);
            break;
        case ValueType::String:
            write_string(out, val.string_value);
            break;
        case ValueType::Boolean:
            write_val(out, val.bool_value);
            break;
        case ValueType::Null:
            // No additional data.
            break;
        case ValueType::Object:
        case ValueType::Vector:
            // Runtime objects should not appear in constant pools.
            throw std::runtime_error("serialize: cannot serialize object/vector constant");
    }
}

Value read_constant(std::istream& in) {
    int32_t type_id = read_val<int32_t>(in);
    ValueType type = static_cast<ValueType>(type_id);

    switch (type) {
        case ValueType::Number: {
            double num = read_val<double>(in);
            return Value::Number(num);
        }
        case ValueType::String: {
            std::string s = read_string(in);
            return Value::String(std::move(s));
        }
        case ValueType::Boolean: {
            bool b = read_val<bool>(in);
            return Value::Boolean(b);
        }
        case ValueType::Null:
            return Value::Null();
        default:
            throw std::runtime_error("serialize: unknown constant type id " + std::to_string(type_id));
    }
}

} // anonymous namespace

void serialize(const BytecodeProgram& prog, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("serialize: cannot open file for writing: " + path);
    }

    // Header: magic + version.
    write_val(out, kSerializeMagic);
    write_val(out, kSerializeVersion);

    // Code section.
    uint32_t code_count = static_cast<uint32_t>(prog.code.size());
    write_val(out, code_count);
    for (const auto& inst : prog.code) {
        write_instruction(out, inst);
    }

    // Constants section.
    uint32_t const_count = static_cast<uint32_t>(prog.constants.size());
    write_val(out, const_count);
    for (const auto& val : prog.constants) {
        write_constant(out, val);
    }

    // Function table section.
    uint32_t func_count = static_cast<uint32_t>(prog.function_table.size());
    write_val(out, func_count);
    for (const auto& entry : prog.function_table) {
        write_string(out, entry.first);
        uint32_t idx = static_cast<uint32_t>(entry.second);
        write_val(out, idx);
    }

    // Type ancestors section (for virtual method dispatch over inherited methods).
    uint32_t anc_count = static_cast<uint32_t>(prog.type_ancestors.size());
    write_val(out, anc_count);
    for (const auto& entry : prog.type_ancestors) {
        write_string(out, entry.first);
        uint32_t n = static_cast<uint32_t>(entry.second.size());
        write_val(out, n);
        for (const auto& anc : entry.second) {
            write_string(out, anc);
        }
    }
}

BytecodeProgram deserialize(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("deserialize: cannot open file for reading: " + path);
    }

    // Validate header.
    uint32_t magic = read_val<uint32_t>(in);
    if (magic != kSerializeMagic) {
        throw std::runtime_error("deserialize: invalid magic number");
    }
    uint32_t version = read_val<uint32_t>(in);
    if (version != kSerializeVersion) {
        throw std::runtime_error("deserialize: unsupported format version " + std::to_string(version));
    }

    BytecodeProgram prog;

    // Code section.
    uint32_t code_count = read_val<uint32_t>(in);
    prog.code.reserve(code_count);
    for (uint32_t i = 0; i < code_count; ++i) {
        prog.code.push_back(read_instruction(in));
    }

    // Constants section.
    uint32_t const_count = read_val<uint32_t>(in);
    prog.constants.reserve(const_count);
    for (uint32_t i = 0; i < const_count; ++i) {
        prog.constants.push_back(read_constant(in));
    }

    // Function table section.
    uint32_t func_count = read_val<uint32_t>(in);
    for (uint32_t i = 0; i < func_count; ++i) {
        std::string name = read_string(in);
        uint32_t idx = read_val<uint32_t>(in);
        prog.function_table[name] = idx;
    }

    // Type ancestors section.
    uint32_t anc_count = read_val<uint32_t>(in);
    for (uint32_t i = 0; i < anc_count; ++i) {
        std::string name = read_string(in);
        uint32_t n = read_val<uint32_t>(in);
        std::vector<std::string> ancestors;
        ancestors.reserve(n);
        for (uint32_t j = 0; j < n; ++j) {
            ancestors.push_back(read_string(in));
        }
        prog.type_ancestors[name] = std::move(ancestors);
    }

    return prog;
}