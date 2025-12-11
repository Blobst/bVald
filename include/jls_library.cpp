#include "jls_library.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>

#include "jq.hpp"

namespace jls {

using FunctionMap = std::map<std::string, ValuePtr>;

static std::string to_lower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
}

static std::string to_upper(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return out;
}

// Registry of library name -> function map (lowercase names inside map)
static std::map<std::string, FunctionMap> library_registry;
static const std::set<std::string> builtin_libraries = {"math", "io", "file",
                                                        "jq"};

static jq::Engine jq_engine;

static void register_library_functions(const std::string &lib_name,
                                       const FunctionMap &functions) {
  FunctionMap normalized;
  for (const auto &kv : functions) {
    normalized[to_lower(kv.first)] = kv.second;
  }
  library_registry[to_lower(lib_name)] = normalized;
}

static void bind_library_to_env(const std::string &lib_name,
                                const FunctionMap &functions,
                                EnvironmentPtr env) {
  if (!env)
    return;

  // Bind individual functions in uppercase for backward compatibility
  for (const auto &kv : functions) {
    env->set(to_upper(kv.first), kv.second);
  }

  // Bind a namespaced map so calls like math/sin() work
  auto lib_val = std::make_shared<Value>();
  lib_val->type = ValueType::MAP;
  for (const auto &kv : functions) {
    lib_val->map[to_lower(kv.first)] = kv.second;
  }
  env->set(to_lower(lib_name), lib_val);
}

// Helper functions for lambda conversion
static ValuePtr math_sin(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::sin(val));
}

static ValuePtr math_cos(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::cos(val));
}

static ValuePtr math_tan(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::tan(val));
}

static ValuePtr math_ln(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::log(val));
}

static ValuePtr math_log(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::log10(val));
}

static ValuePtr math_exp(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(1.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::exp(val));
}

static ValuePtr math_round(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);
  double val = (args[0]->type == ValueType::INTEGER) ? args[0]->i : args[0]->f;
  return std::make_shared<Value>(std::round(val));
}

static ValuePtr io_printno(const std::vector<ValuePtr> &args) {
  for (const auto &arg : args) {
    if (arg->type == ValueType::STRING) {
      std::cout << arg->s;
    } else if (arg->type == ValueType::INTEGER) {
      std::cout << arg->i;
    } else if (arg->type == ValueType::FLOAT) {
      std::cout << arg->f;
    } else if (arg->type == ValueType::BOOLEAN) {
      std::cout << (arg->b ? "true" : "false");
    }
  }
  return std::make_shared<Value>();
}

static ValuePtr io_pause(const std::vector<ValuePtr> &args) {
  std::string prompt = "Press any key to continue...";
  if (!args.empty() && args[0]->type == ValueType::STRING) {
    prompt = args[0]->s;
  }
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  return std::make_shared<Value>();
}

static ValuePtr file_read(const std::vector<ValuePtr> &args) {
  if (args.empty() || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>("");
  }

  std::ifstream file(args[0]->s);
  if (!file.is_open()) {
    return std::make_shared<Value>("");
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();
  return std::make_shared<Value>(content);
}

static ValuePtr file_write(const std::vector<ValuePtr> &args) {
  if (args.size() < 2 || args[0]->type != ValueType::STRING ||
      args[1]->type != ValueType::STRING) {
    return std::make_shared<Value>(false);
  }

  std::ofstream file(args[0]->s);
  if (!file.is_open()) {
    return std::make_shared<Value>(false);
  }

  file << args[1]->s;
  file.close();
  return std::make_shared<Value>(true);
}

static ValuePtr file_exists(const std::vector<ValuePtr> &args) {
  if (args.empty() || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>(false);
  }

  std::ifstream file(args[0]->s);
  bool exists = file.good();
  file.close();
  return std::make_shared<Value>(exists);
}

static ValuePtr jq_run(const std::vector<ValuePtr> &args) {
  if (args.size() < 2 || args[0]->type != ValueType::STRING ||
      args[1]->type != ValueType::STRING) {
    return std::make_shared<Value>("[JQ ERROR] expected (filter, json_string)");
  }

  std::string output;
  std::string err;
  if (!jq_engine.run(args[0]->s, args[1]->s, output, err)) {
    return std::make_shared<Value>("[JQ ERROR] " + err);
  }

  return std::make_shared<Value>(output);
}

static ValuePtr jq_keys(const std::vector<ValuePtr> &args) {
  if (args.size() < 1 || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>("[JQ ERROR] jq/keys expected (json_string)");
  }

  std::vector<std::string> outputs;
  std::string err;
  if (!jq_engine.run_streaming("keys", args[0]->s, outputs, err)) {
    return std::make_shared<Value>("[JQ ERROR] " + err);
  }

  return std::make_shared<Value>(outputs.empty() ? "null" : outputs[0]);
}

static ValuePtr jq_values(const std::vector<ValuePtr> &args) {
  if (args.size() < 1 || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>(
        "[JQ ERROR] jq/values expected (json_string)");
  }

  std::vector<std::string> outputs;
  std::string err;
  if (!jq_engine.run_streaming("values", args[0]->s, outputs, err)) {
    return std::make_shared<Value>("[JQ ERROR] " + err);
  }

  return std::make_shared<Value>(outputs.empty() ? "null" : outputs[0]);
}

static ValuePtr jq_type(const std::vector<ValuePtr> &args) {
  if (args.size() < 1 || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>("[JQ ERROR] jq/type expected (json_string)");
  }

  std::vector<std::string> outputs;
  std::string err;
  if (!jq_engine.run_streaming("type", args[0]->s, outputs, err)) {
    return std::make_shared<Value>("[JQ ERROR] " + err);
  }

  return std::make_shared<Value>(outputs.empty() ? "null" : outputs[0]);
}

static ValuePtr jq_length(const std::vector<ValuePtr> &args) {
  if (args.size() < 1 || args[0]->type != ValueType::STRING) {
    return std::make_shared<Value>(
        "[JQ ERROR] jq/length expected (json_string)");
  }

  std::vector<std::string> outputs;
  std::string err;
  if (!jq_engine.run_streaming("length", args[0]->s, outputs, err)) {
    return std::make_shared<Value>("[JQ ERROR] " + err);
  }

  return std::make_shared<Value>(outputs.empty() ? "null" : outputs[0]);
}

static FunctionMap make_math_functions() {
  FunctionMap funcs;

  auto sin_val = std::make_shared<Value>();
  sin_val->type = ValueType::FUNCTION;
  sin_val->native_func = math_sin;
  funcs["sin"] = sin_val;

  auto cos_val = std::make_shared<Value>();
  cos_val->type = ValueType::FUNCTION;
  cos_val->native_func = math_cos;
  funcs["cos"] = cos_val;

  auto tan_val = std::make_shared<Value>();
  tan_val->type = ValueType::FUNCTION;
  tan_val->native_func = math_tan;
  funcs["tan"] = tan_val;

  auto ln_val = std::make_shared<Value>();
  ln_val->type = ValueType::FUNCTION;
  ln_val->native_func = math_ln;
  funcs["ln"] = ln_val;

  auto log_val = std::make_shared<Value>();
  log_val->type = ValueType::FUNCTION;
  log_val->native_func = math_log;
  funcs["log"] = log_val;

  auto exp_val = std::make_shared<Value>();
  exp_val->type = ValueType::FUNCTION;
  exp_val->native_func = math_exp;
  funcs["exp"] = exp_val;

  auto round_val = std::make_shared<Value>();
  round_val->type = ValueType::FUNCTION;
  round_val->native_func = math_round;
  funcs["round"] = round_val;

  auto pi_val = std::make_shared<Value>();
  pi_val->type = ValueType::FLOAT;
  pi_val->f = 3.141592653589793;
  funcs["pi"] = pi_val;

  auto e_val = std::make_shared<Value>();
  e_val->type = ValueType::FLOAT;
  e_val->f = 2.718281828459045;
  funcs["e"] = e_val;

  return funcs;
}

static FunctionMap make_io_functions() {
  FunctionMap funcs;

  auto printno_val = std::make_shared<Value>();
  printno_val->type = ValueType::FUNCTION;
  printno_val->native_func = io_printno;
  funcs["printno"] = printno_val;

  auto pause_val = std::make_shared<Value>();
  pause_val->type = ValueType::FUNCTION;
  pause_val->native_func = io_pause;
  funcs["pause"] = pause_val;

  return funcs;
}

static FunctionMap make_file_functions() {
  FunctionMap funcs;

  auto read_file_val = std::make_shared<Value>();
  read_file_val->type = ValueType::FUNCTION;
  read_file_val->native_func = file_read;
  funcs["read_file"] = read_file_val;

  auto write_file_val = std::make_shared<Value>();
  write_file_val->type = ValueType::FUNCTION;
  write_file_val->native_func = file_write;
  funcs["write_file"] = write_file_val;

  auto file_exists_val = std::make_shared<Value>();
  file_exists_val->type = ValueType::FUNCTION;
  file_exists_val->native_func = file_exists;
  funcs["file_exists"] = file_exists_val;

  return funcs;
}

static FunctionMap make_jq_functions() {
  FunctionMap funcs;

  auto run_val = std::make_shared<Value>();
  run_val->type = ValueType::FUNCTION;
  run_val->native_func = jq_run;
  auto keys_val = std::make_shared<Value>();
  keys_val->type = ValueType::FUNCTION;
  keys_val->native_func = jq_keys;
  funcs["keys"] = keys_val;

  auto values_val = std::make_shared<Value>();
  values_val->type = ValueType::FUNCTION;
  values_val->native_func = jq_values;
  funcs["values"] = values_val;

  auto type_val = std::make_shared<Value>();
  type_val->type = ValueType::FUNCTION;
  type_val->native_func = jq_type;
  funcs["type"] = type_val;

  auto length_val = std::make_shared<Value>();
  length_val->type = ValueType::FUNCTION;
  length_val->native_func = jq_length;
  funcs["length"] = length_val;

  funcs["run"] = run_val;

  return funcs;
}

bool LibraryLoader::load_library(const std::string &lib_name,
                                 EnvironmentPtr env) {
  std::string lower_name = to_lower(lib_name);

  if (lower_name == "math") {
    return load_math_library(env);
  }
  if (lower_name == "io") {
    return load_io_library(env);
  }
  if (lower_name == "file") {
    return load_file_library(env);
  }
  if (lower_name == "jq") {
    return load_jq_library(env);
  }

  auto it = library_registry.find(lower_name);
  if (it != library_registry.end()) {
    bind_library_to_env(lower_name, it->second, env);
    return true;
  }

  return false;
}

std::vector<std::string> LibraryLoader::get_available_libraries() {
  std::vector<std::string> libs = {"math", "io", "file", "jq"};
  for (const auto &kv : library_registry) {
    if (std::find(libs.begin(), libs.end(), kv.first) == libs.end()) {
      libs.push_back(kv.first);
    }
  }
  return libs;
}

void LibraryLoader::register_custom_library(
    const std::string &lib_name,
    const std::map<std::string, NativeFunctionPtr> &functions) {
  FunctionMap map;
  for (const auto &kv : functions) {
    auto fn_val = std::make_shared<Value>();
    fn_val->type = ValueType::FUNCTION;
    fn_val->native_func = kv.second;
    map[kv.first] = fn_val;
  }
  register_library_functions(lib_name, map);
}

std::vector<std::string>
LibraryLoader::get_library_functions(const std::string &lib_name) {
  std::string lower = to_lower(lib_name);

  if (lower == "math" &&
      library_registry.find(lower) == library_registry.end()) {
    register_library_functions(lower, make_math_functions());
  } else if (lower == "io" &&
             library_registry.find(lower) == library_registry.end()) {
    register_library_functions(lower, make_io_functions());
  } else if (lower == "file" &&
             library_registry.find(lower) == library_registry.end()) {
    register_library_functions(lower, make_file_functions());
  } else if (lower == "jq" &&
             library_registry.find(lower) == library_registry.end()) {
    register_library_functions(lower, make_jq_functions());
  }

  auto it = library_registry.find(lower);
  if (it == library_registry.end()) {
    return {};
  }

  std::vector<std::string> names;
  for (const auto &kv : it->second) {
    names.push_back(kv.first);
  }
  std::sort(names.begin(), names.end());
  return names;
}

bool LibraryLoader::is_library_name(const std::string &lib_name) {
  std::string lower = to_lower(lib_name);
  if (builtin_libraries.count(lower)) {
    return true;
  }
  return library_registry.find(lower) != library_registry.end();
}

bool LibraryLoader::load_math_library(EnvironmentPtr env) {
  auto funcs = make_math_functions();
  register_library_functions("math", funcs);
  bind_library_to_env("math", library_registry["math"], env);
  return true;
}

bool LibraryLoader::load_io_library(EnvironmentPtr env) {
  auto funcs = make_io_functions();
  register_library_functions("io", funcs);
  bind_library_to_env("io", library_registry["io"], env);
  return true;
}

bool LibraryLoader::load_file_library(EnvironmentPtr env) {
  auto funcs = make_file_functions();
  register_library_functions("file", funcs);
  bind_library_to_env("file", library_registry["file"], env);
  return true;
}

bool LibraryLoader::load_jq_library(EnvironmentPtr env) {
  auto funcs = make_jq_functions();
  register_library_functions("jq", funcs);
  bind_library_to_env("jq", library_registry["jq"], env);
  return true;
}

} // namespace jls
