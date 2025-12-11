#ifndef JQ_EXECUTOR_HPP
#define JQ_EXECUTOR_HPP

#include "jq_bytecode.hpp"
#include "jq_types.hpp"
#include <vector>

namespace jq {

// Stream-based executor: a single filter can yield multiple outputs.
class Executor {
public:
  // Execute program against input and collect outputs.
  // Returns true if execution succeeds; outputs holds result values.
  bool execute(const Program &prog, const JvValuePtr &input,
               std::vector<JvValuePtr> &outputs, std::string &err);

private:
  // Helper to execute a range of instructions with a given current value.
  bool exec_range(const Program &prog, size_t start, size_t end,
                  const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err);
};

} // namespace jq

#endif // JQ_EXECUTOR_HPP
