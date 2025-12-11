#include "jq_bytecode.hpp"
#include <iostream>

namespace jq {

// Pretty-print a single instruction for debugging
std::string instruction_to_string(const Instruction &ins,
                                  const ConstantPool &pool) {
  std::string result;
  switch (ins.op) {
  case OpCode::NOP:
    result = "NOP";
    break;
  case OpCode::LOAD_IDENTITY:
    result = "LOAD_IDENTITY";
    break;
  case OpCode::GET_FIELD:
    result = "GET_FIELD";
    if (ins.a >= 0 && static_cast<size_t>(ins.a) < pool.strings.size()) {
      result += " \"" + pool.strings[ins.a] + "\"";
    }
    break;
  case OpCode::GET_INDEX_NUM:
    result = "GET_INDEX_NUM";
    if (ins.a >= 0 && static_cast<size_t>(ins.a) < pool.numbers.size()) {
      result += " " + std::to_string(pool.numbers[ins.a]);
    }
    break;
  case OpCode::GET_INDEX_STR:
    result = "GET_INDEX_STR";
    if (ins.a >= 0 && static_cast<size_t>(ins.a) < pool.strings.size()) {
      result += " \"" + pool.strings[ins.a] + "\"";
    }
    break;
  case OpCode::ITERATE:
    result = "ITERATE";
    break;
  case OpCode::ADD_CONST:
    result = "ADD_CONST";
    if (ins.a >= 0 && static_cast<size_t>(ins.a) < pool.numbers.size()) {
      result += " " + std::to_string(pool.numbers[ins.a]);
    }
    break;
  case OpCode::LENGTH:
    result = "LENGTH";
    break;
  case OpCode::BUILTIN_CALL:
    result = "BUILTIN_CALL";
    if (ins.a >= 0 && static_cast<size_t>(ins.a) < pool.strings.size()) {
      result += " \"" + pool.strings[ins.a] + "\"";
    }
    break;
  default:
    result = "UNKNOWN";
    break;
  }
  return result;
}

// Disassemble and print a program (debugging utility)
void print_program(const Program &prog, std::ostream &out) {
  out << "=== Program Disassembly ===\n";
  out << "Constant Pool:\n";

  out << "  Strings:\n";
  for (size_t i = 0; i < prog.pool.strings.size(); ++i) {
    out << "    [" << i << "] \"" << prog.pool.strings[i] << "\"\n";
  }

  out << "  Numbers:\n";
  for (size_t i = 0; i < prog.pool.numbers.size(); ++i) {
    out << "    [" << i << "] " << prog.pool.numbers[i] << "\n";
  }

  out << "\nInstructions:\n";
  for (size_t i = 0; i < prog.code.size(); ++i) {
    out << "  [" << i << "] " << instruction_to_string(prog.code[i], prog.pool)
        << "\n";
  }
  out << "==========================\n";
}

} // namespace jq
