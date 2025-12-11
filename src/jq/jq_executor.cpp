#include "jq_executor.hpp"
#include "jq_builtins.hpp"

namespace jq {

bool Executor::execute(const Program &prog, const JvValuePtr &input,
                       std::vector<JvValuePtr> &outputs, std::string &err) {
  outputs.clear();
  return exec_range(prog, 0, prog.code.size(), input, outputs, err);
}

bool Executor::exec_range(const Program &prog, size_t start, size_t end,
                          const JvValuePtr &input,
                          std::vector<JvValuePtr> &outputs, std::string &err) {
  JvValuePtr current = input;

  for (size_t i = start; i < end; ++i) {
    const auto &ins = prog.code[i];

    switch (ins.op) {
    case OpCode::LOAD_IDENTITY:
      // Keep current value
      break;

    case OpCode::GET_FIELD: {
      if (!current || !current->is_object()) {
        current = JvValue::null();
      } else {
        const auto &key = prog.pool.strings[static_cast<size_t>(ins.a)];
        current = current->object_get(key);
      }
      break;
    }

    case OpCode::GET_INDEX_STR: {
      if (!current || !current->is_object()) {
        current = JvValue::null();
      } else {
        const auto &key = prog.pool.strings[static_cast<size_t>(ins.a)];
        current = current->object_get(key);
      }
      break;
    }

    case OpCode::GET_INDEX_NUM: {
      if (!current || !current->is_array()) {
        current = JvValue::null();
      } else {
        double idx = prog.pool.numbers[static_cast<size_t>(ins.a)];
        size_t i = static_cast<size_t>(idx);
        current = current->array_index(i);
      }
      break;
    }

    case OpCode::ITERATE: {
      // Expand array into multiple outputs
      if (!current || !current->is_array()) {
        // Non-arrays pass through as single output
        outputs.push_back(current ? current : JvValue::null());
      } else {
        // Array iterates each element
        for (const auto &elem : current->a) {
          outputs.push_back(elem);
        }
      }
      return true;
    }

    case OpCode::ADD_CONST: {
      if (!current || !current->is_number()) {
        current = JvValue::null();
      } else {
        double k = prog.pool.numbers[static_cast<size_t>(ins.a)];
        current = JvValue::number(current->n + k);
      }
      break;
    }

    case OpCode::LENGTH: {
      if (!current) {
        current = JvValue::number(0);
      } else if (current->is_string()) {
        current = JvValue::number(static_cast<double>(current->s.size()));
      } else if (current->is_array()) {
        current = JvValue::number(static_cast<double>(current->a.size()));
      } else if (current->is_object()) {
        current = JvValue::number(static_cast<double>(current->o.size()));
      } else {
        current = JvValue::number(0);
      }
      break;
    }

    case OpCode::BUILTIN_CALL: {
      // ins.a holds string pool index of builtin name
      const auto &name = prog.pool.strings[static_cast<size_t>(ins.a)];
      std::vector<JvValuePtr> builtin_outputs;
      if (!Builtins::call_builtin(name, current, builtin_outputs, err)) {
        return false;
      }
      // Replace current with first output if any; add rest to outputs
      if (!builtin_outputs.empty()) {
        current = builtin_outputs[0];
        for (size_t j = 1; j < builtin_outputs.size(); ++j) {
          outputs.push_back(builtin_outputs[j]);
        }
      } else {
        current = JvValue::null();
      }
      break;
    }

    default:
      err = "Unknown opcode";
      return false;
    }
  }

  outputs.push_back(current ? current : JvValue::null());
  return true;
}

} // namespace jq
