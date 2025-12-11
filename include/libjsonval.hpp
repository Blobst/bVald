#ifndef LIBJSONVAL_HPP
#define LIBJSONVAL_HPP

#include <fstream>
#include <iostream>
#include <map>
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

#endif // LIBJSONVAL_HPP
