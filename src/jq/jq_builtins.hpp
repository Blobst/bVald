#ifndef JQ_BUILTINS_HPP
#define JQ_BUILTINS_HPP

#include "jq_types.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace jq {

// Builtin function signature: takes input value(s) and returns outputs.
using BuiltinFunc = std::function<bool(
    const JvValuePtr &, std::vector<JvValuePtr> &, std::string &)>;

class Builtins {
public:
  // Register a builtin function
  static void register_builtin(const std::string &name, const BuiltinFunc &fn);

  // Look up a builtin by name
  static bool has_builtin(const std::string &name);
  static BuiltinFunc get_builtin(const std::string &name);

  // Call a builtin
  static bool call_builtin(const std::string &name, const JvValuePtr &input,
                           std::vector<JvValuePtr> &outputs, std::string &err);

private:
  static std::map<std::string, BuiltinFunc> registry_;
  static bool initialized_;

  // Initialize standard builtins
  static void init_builtins();
};

// Builtin implementations
namespace builtins {

// keys: returns array of object keys or array indices
bool keys_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err);

// values: returns object values or array elements
bool values_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err);

// type: returns type name as string
bool type_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err);

// length: returns length of string/array/object
bool length_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err);

// map(f): apply filter to each array element (simplified: needs filter
// argument)
bool map_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                 std::string &err);

// select(f): filter based on condition (simplified: needs filter argument)
bool select_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err);

// empty: produces no output
bool empty_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                   std::string &err);

// reverse: reverse array or string
bool reverse_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                     std::string &err);

// sort: sort array
bool sort_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err);

// to_entries: convert object to [key,value] array
bool to_entries_builtin(const JvValuePtr &input,
                        std::vector<JvValuePtr> &outputs, std::string &err);

} // namespace builtins

} // namespace jq

#endif // JQ_BUILTINS_HPP
