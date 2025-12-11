#include "../../include/jq.hpp"
#include "../../include/libjsonval.hpp"
#include "jq_builtins.hpp"
#include "jq_bytecode.hpp"
#include "jq_compiler.hpp"
#include "jq_executor.hpp"
#include "jq_lexer.hpp"
#include "jq_parser.hpp"
#include "jq_types.hpp"

namespace jq {

Engine::Engine() = default;

bool Engine::compile(const std::string &filter, std::string &err) {
  if (filter.empty()) {
    err = "jq filter cannot be empty";
    return false;
  }

  try {
    // Tokenize the filter
    Lexer lexer(filter);
    auto tokens = lexer.tokenize();

    // Parse tokens into AST
    Parser parser(tokens);
    ast_ = parser.parse();

    if (!ast_) {
      err = "Failed to parse jq filter";
      return false;
    }

    // Compile AST to bytecode Program using Compiler
    auto prog_ptr = std::make_shared<Program>();
    Compiler cc;
    if (!cc.compile(ast_, *prog_ptr, err)) {
      return false;
    }
    program_ = prog_ptr;

    return true;
  } catch (const std::exception &e) {
    err = std::string("Parse error: ") + e.what();
    return false;
  }
}

bool Engine::run(const std::string &filter, const std::string &json_in,
                 std::string &json_out, std::string &err) {
  std::vector<std::string> outputs;
  if (!run_streaming(filter, json_in, outputs, err)) {
    return false;
  }
  json_out = outputs.empty() ? "null" : outputs[0];
  return true;
}

bool Engine::run_streaming(const std::string &filter,
                           const std::string &json_in,
                           std::vector<std::string> &json_outputs,
                           std::string &err) {
  if (!compile(filter, err)) {
    return false;
  }

  auto jv = JvValue::from_string(json_in, err);
  if (!jv) {
    err = "Invalid JSON input";
    return false;
  }

  Executor executor;
  std::vector<JvValuePtr> outputs;
  if (!executor.execute(*program_, jv, outputs, err)) {
    return false;
  }

  json_outputs.clear();
  for (const auto &out : outputs) {
    json_outputs.push_back(out ? out->to_string() : "null");
  }
  return true;
}

void Engine::register_builtin(
    const std::string &name,
    const std::function<bool(const JvValuePtr &, std::vector<JvValuePtr> &,
                             std::string &)> &fn) {
  Builtins::register_builtin(name, fn);
}

} // namespace jq
