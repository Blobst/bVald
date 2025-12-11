#ifndef BVALD_JQ_HPP
#define BVALD_JQ_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace jq {
struct ASTNode;
struct Program;
class JvValue;
using JvValuePtr = std::shared_ptr<JvValue>;
class Builtins;
} // namespace jq

namespace jq {

// Streaming JSON query engine with full bytecode compilation and execution.
class Engine {
public:
  Engine();
  ~Engine() = default;

  // Compile a jq filter into an AST and bytecode
  bool compile(const std::string &filter, std::string &err);

  // Run a compiled filter against JSON text (returns first output for
  // compatibility)
  bool run(const std::string &filter, const std::string &json_in,
           std::string &json_out, std::string &err);

  // Run filter and collect all outputs
  bool run_streaming(const std::string &filter, const std::string &json_in,
                     std::vector<std::string> &json_outputs, std::string &err);

  // Register a custom builtin function
  static void register_builtin(
      const std::string &name,
      const std::function<bool(const JvValuePtr &, std::vector<JvValuePtr> &,
                               std::string &)> &fn);

private:
  std::shared_ptr<ASTNode> ast_;
  std::shared_ptr<Program> program_;
};

} // namespace jq

#endif // BVALD_JQ_HPP
