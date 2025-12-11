#include "jq_builtins.hpp"
#include <algorithm>

namespace jq {

std::map<std::string, BuiltinFunc> Builtins::registry_;
bool Builtins::initialized_ = false;

void Builtins::init_builtins() {
  if (initialized_)
    return;
  initialized_ = true;

  register_builtin("keys", builtins::keys_builtin);
  register_builtin("values", builtins::values_builtin);
  register_builtin("type", builtins::type_builtin);
  register_builtin("length", builtins::length_builtin);
  register_builtin("empty", builtins::empty_builtin);
  register_builtin("reverse", builtins::reverse_builtin);
  register_builtin("sort", builtins::sort_builtin);
  register_builtin("to_entries", builtins::to_entries_builtin);
}

void Builtins::register_builtin(const std::string &name,
                                const BuiltinFunc &fn) {
  init_builtins();
  registry_[name] = fn;
}

bool Builtins::has_builtin(const std::string &name) {
  init_builtins();
  return registry_.count(name) > 0;
}

BuiltinFunc Builtins::get_builtin(const std::string &name) {
  init_builtins();
  auto it = registry_.find(name);
  if (it != registry_.end())
    return it->second;
  return nullptr;
}

bool Builtins::call_builtin(const std::string &name, const JvValuePtr &input,
                            std::vector<JvValuePtr> &outputs,
                            std::string &err) {
  init_builtins();
  auto fn = get_builtin(name);
  if (!fn) {
    err = "Unknown builtin: " + name;
    return false;
  }
  return fn(input, outputs, err);
}

namespace builtins {

bool keys_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err) {
  if (!input) {
    err = "keys: null input";
    return false;
  }

  auto result = JvValue::array();
  if (input->is_object()) {
    for (const auto &kv : input->o) {
      result->array_push(JvValue::string(kv.first));
    }
  } else if (input->is_array()) {
    for (size_t i = 0; i < input->a.size(); ++i) {
      result->array_push(JvValue::number(static_cast<double>(i)));
    }
  } else {
    err = "keys: input must be object or array";
    return false;
  }

  outputs.push_back(result);
  return true;
}

bool values_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err) {
  if (!input) {
    err = "values: null input";
    return false;
  }

  if (input->is_object()) {
    for (const auto &kv : input->o) {
      outputs.push_back(kv.second);
    }
  } else if (input->is_array()) {
    for (const auto &elem : input->a) {
      outputs.push_back(elem);
    }
  } else {
    err = "values: input must be object or array";
    return false;
  }

  return true;
}

bool type_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err) {
  std::string type_name;
  if (!input || input->is_null()) {
    type_name = "null";
  } else if (input->is_bool()) {
    type_name = "boolean";
  } else if (input->is_number()) {
    type_name = "number";
  } else if (input->is_string()) {
    type_name = "string";
  } else if (input->is_array()) {
    type_name = "array";
  } else if (input->is_object()) {
    type_name = "object";
  } else {
    type_name = "unknown";
  }

  outputs.push_back(JvValue::string(type_name));
  return true;
}

bool length_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err) {
  if (!input) {
    outputs.push_back(JvValue::number(0));
    return true;
  }

  if (input->is_string()) {
    outputs.push_back(JvValue::number(static_cast<double>(input->s.size())));
  } else if (input->is_array()) {
    outputs.push_back(JvValue::number(static_cast<double>(input->a.size())));
  } else if (input->is_object()) {
    outputs.push_back(JvValue::number(static_cast<double>(input->o.size())));
  } else if (input->is_null()) {
    outputs.push_back(JvValue::number(0));
  } else {
    outputs.push_back(JvValue::number(0));
  }

  return true;
}

bool map_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                 std::string &err) {
  err = "map: not yet implemented (requires filter argument support)";
  return false;
}

bool select_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                    std::string &err) {
  err = "select: not yet implemented (requires filter argument support)";
  return false;
}

bool empty_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                   std::string &err) {
  // empty produces no output
  return true;
}

bool reverse_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                     std::string &err) {
  if (!input) {
    outputs.push_back(JvValue::null());
    return true;
  }

  if (input->is_string()) {
    std::string rev = input->s;
    std::reverse(rev.begin(), rev.end());
    outputs.push_back(JvValue::string(rev));
  } else if (input->is_array()) {
    auto arr = JvValue::array();
    for (auto it = input->a.rbegin(); it != input->a.rend(); ++it) {
      arr->array_push(*it);
    }
    outputs.push_back(arr);
  } else {
    err = "reverse: input must be string or array";
    return false;
  }

  return true;
}

bool sort_builtin(const JvValuePtr &input, std::vector<JvValuePtr> &outputs,
                  std::string &err) {
  if (!input || !input->is_array()) {
    err = "sort: input must be array";
    return false;
  }

  auto sorted = JvValue::array();
  std::vector<JvValuePtr> elems = input->a;

  std::sort(elems.begin(), elems.end(),
            [](const JvValuePtr &a, const JvValuePtr &b) -> bool {
              // Simple comparison: numbers < strings < others
              if (a->is_number() && b->is_number())
                return a->n < b->n;
              if (a->is_string() && b->is_string())
                return a->s < b->s;
              // Mixed types: use type order
              auto type_order = [](const JvValuePtr &v) -> int {
                if (v->is_null())
                  return 0;
                if (v->is_bool())
                  return 1;
                if (v->is_number())
                  return 2;
                if (v->is_string())
                  return 3;
                if (v->is_array())
                  return 4;
                if (v->is_object())
                  return 5;
                return 6;
              };
              return type_order(a) < type_order(b);
            });

  for (const auto &elem : elems) {
    sorted->array_push(elem);
  }

  outputs.push_back(sorted);
  return true;
}

bool to_entries_builtin(const JvValuePtr &input,
                        std::vector<JvValuePtr> &outputs, std::string &err) {
  if (!input || !input->is_object()) {
    err = "to_entries: input must be object";
    return false;
  }

  auto result = JvValue::array();
  for (const auto &kv : input->o) {
    auto entry = JvValue::object();
    entry->object_set("key", JvValue::string(kv.first));
    entry->object_set("value", kv.second);
    result->array_push(entry);
  }

  outputs.push_back(result);
  return true;
}

} // namespace builtins

} // namespace jq
