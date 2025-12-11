#ifndef LIBJSONVAL_HPP
#define LIBJSONVAL_HPP

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// ================= DLL Export Macro =================
#ifdef _WIN32
#ifdef JSONVAL_EXPORTS // Defined when building the DLL
#define JSONVAL_API __declspec(dllexport)
#else // Used when importing the DLL
#define JSONVAL_API __declspec(dllimport)
#endif
#else
#define JSONVAL_API
#endif

// ================= Library Version ==================
extern JSONVAL_API const std::string VERSION;

// ================= JSON DOM =========================
struct JSONVAL_API JsonValue {
  enum Type { T_NULL, T_BOOL, T_NUMBER, T_STRING, T_OBJECT, T_ARRAY } t;
  bool b = false;
  double n = 0.0;
  std::string s;
  std::map<std::string, JsonValue> o;
  std::vector<JsonValue> a;

  JsonValue() : t(T_NULL) {}
};

// Parse JSON text into a JsonValue tree. Returns true on success, false on
// error with details in `err`.
JSONVAL_API bool parse_json_dom(const std::string &json_text, JsonValue &out,
                                std::string &err);

// Print JSON as a tree structure to stdout.
JSONVAL_API void print_json_tree(const JsonValue &val,
                                 const std::string &prefix = "",
                                 bool is_last = true);

/**
 * Validate a JSON string.
 *
 * @param text       JSON string to validate
 * @param error_msg  Output for detailed error message if validation fails
 * @return true if valid JSON, false otherwise
 */
JSONVAL_API bool validate_json(const std::string &text, std::string &error_msg);

/**
 * Print usage information.
 *
 * @param program_name executable name
 */
JSONVAL_API void print_help(const char *program_name);

// ================= Schema registry ==================
struct JSONVAL_API SchemaEntry {
  std::string id;
  std::string name;
  std::string description;
  std::string source; // local file path or HTTP(S) URL
  std::string schemaVersion;
  std::vector<std::string> links;
};

// Initialize schema registry from a `schemas.json` file.
// Returns true on success.
JSONVAL_API bool init_schema_registry(const std::string &config_path,
                                      std::string &err);

// Get schema source content by id or source string (id|url|path).
// Returns true on success.
JSONVAL_API bool get_schema_source(const std::string &id_or_source,
                                   std::string &out, std::string &err);

// Resolve schema and all linked schemas recursively into a map of id->content.
JSONVAL_API bool
resolve_schema_links(const std::string &id_or_source,
                     std::map<std::string, std::string> &out_map,
                     std::string &err);

// Return list of known schema ids.
JSONVAL_API std::vector<std::string> list_schema_ids();

/**
 * Validate JSON using minimal JSON Schema subset:
 * supports: type, properties, required, items, enum.
 */
JSONVAL_API bool validate_json_with_schema(const std::string &json_text,
                                           const std::string &schema_text,
                                           std::string &err);

// ================= jq JSON Query Engine ==================

// Forward declarations for jq components
namespace jq {
class JvValue;
using JvValuePtr = std::shared_ptr<JvValue>;

class Lexer;
class Parser;
struct ASTNode;

class Compiler;
struct Program;
struct Instruction;
enum class OpCode : uint16_t;

class Executor;
class Builtins;
class Engine;
} // namespace jq

// Re-export jq components in global namespace for convenience
using JqLexer = jq::Lexer;
using JqParser = jq::Parser;
using JqASTNode = jq::ASTNode;
using JqCompiler = jq::Compiler;
using JqProgram = jq::Program;
using JqInstruction = jq::Instruction;
using JqOpCode = jq::OpCode;
using JqExecutor = jq::Executor;
using JqBuiltins = jq::Builtins;
using JqEngine = jq::Engine;
using JqValue = jq::JvValue;
using JqValuePtr = jq::JvValuePtr;

// Include jq component headers for full API access
#include "../src/jq/jq_builtins.hpp"
#include "../src/jq/jq_bytecode.hpp"
#include "../src/jq/jq_compiler.hpp"
#include "../src/jq/jq_executor.hpp"
#include "../src/jq/jq_lexer.hpp"
#include "../src/jq/jq_parser.hpp"
#include "../src/jq/jq_types.hpp"
#include "jq.hpp"

/**
 * Compile and run a jq filter against JSON input.
 *
 * @param filter   jq filter string (e.g., ".name", ".[] | .age + 1")
 * @param json_in  JSON input string
 * @param json_out Output JSON string (first result for compatibility)
 * @param err      Error message if execution fails
 * @return true on success, false on error
 *
 * Example:
 *   std::string output, error;
 *   if (run_jq_filter(".name", R"({"name": "Alice"})", output, error)) {
 *     std::cout << output << std::endl;  // prints: "Alice"
 *   }
 */
JSONVAL_API bool run_jq_filter(const std::string &filter,
                               const std::string &json_in,
                               std::string &json_out, std::string &err);

/**
 * Compile and run a jq filter, collecting all outputs.
 *
 * @param filter      jq filter string
 * @param json_in     JSON input string
 * @param json_outputs Vector of JSON output strings
 * @param err         Error message if execution fails
 * @return true on success, false on error
 *
 * Example:
 *   std::vector<std::string> outputs;
 *   std::string error;
 *   if (run_jq_filter_streaming(".[]", "[1, 2, 3]", outputs, error)) {
 *     for (const auto &out : outputs) {
 *       std::cout << out << std::endl;  // prints: 1, 2, 3
 *     }
 *   }
 */
JSONVAL_API bool run_jq_filter_streaming(const std::string &filter,
                                         const std::string &json_in,
                                         std::vector<std::string> &json_outputs,
                                         std::string &err);

/**
 * Register a custom jq builtin function.
 *
 * The function takes a JSON input and produces zero or more JSON outputs.
 *
 * @param name Function name (e.g., "double", "square")
 * @param fn   Lambda/function that processes input and populates outputs
 *
 * Example:
 *   auto double_fn = [](const std::string &json_in,
 *                       std::vector<std::string> &outputs,
 *                       std::string &err) -> bool {
 *     try {
 *       double val = std::stod(json_in);
 *       outputs.push_back(std::to_string(val * 2));
 *       return true;
 *     } catch (...) {
 *       err = "Not a number";
 *       return false;
 *     }
 *   };
 *   register_jq_builtin("double", double_fn);
 */
JSONVAL_API void register_jq_builtin(
    const std::string &name,
    const std::function<bool(const std::string &, std::vector<std::string> &,
                             std::string &)> &fn);

#endif // LIBJSONVAL_HPP
