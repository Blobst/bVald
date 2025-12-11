#ifndef JQ_BYTECODE_HPP
#define JQ_BYTECODE_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace jq {

// Canonical jq opcodes (subset). Extend as needed.
enum class OpCode : uint16_t {
  NOP = 0,
  LOAD_IDENTITY,
  GET_FIELD,
  GET_INDEX_NUM,
  GET_INDEX_STR,
  ITERATE,
  ADD_CONST,
  LENGTH,
  BUILTIN_CALL,
};

struct Instruction {
  OpCode op = OpCode::NOP;
  int32_t a = -1; // general operand (e.g., pool index)
  int32_t b = -1; // optional operand
};

struct ConstantPool {
  std::vector<std::string> strings;
  std::vector<double> numbers;

  int add_string(const std::string &s) {
    strings.push_back(s);
    return static_cast<int>(strings.size() - 1);
  }
  int add_number(double v) {
    numbers.push_back(v);
    return static_cast<int>(numbers.size() - 1);
  }
};

struct Program {
  std::vector<Instruction> code;
  ConstantPool pool;

  bool validate(std::string &err) const {
    for (size_t i = 0; i < code.size(); ++i) {
      const auto &ins = code[i];
      switch (ins.op) {
      case OpCode::GET_FIELD:
      case OpCode::GET_INDEX_STR:
        if (ins.a < 0 || static_cast<size_t>(ins.a) >= pool.strings.size()) {
          err = "Invalid string pool index in instruction at pc=" +
                std::to_string(i);
          return false;
        }
        break;
      case OpCode::GET_INDEX_NUM:
      case OpCode::ADD_CONST:
        if (ins.a < 0 || static_cast<size_t>(ins.a) >= pool.numbers.size()) {
          err = "Invalid number pool index in instruction at pc=" +
                std::to_string(i);
          return false;
        }
        break;
      default:
        break;
      }
    }
    return true;
  }
};

// Bytecode debugging and utility functions
std::string instruction_to_string(const Instruction &ins,
                                  const ConstantPool &pool);
void print_program(const Program &prog, std::ostream &out = std::cout);

} // namespace jq

#endif // JQ_BYTECODE_HPP