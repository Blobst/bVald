#ifndef LIBJSONVAL_HPP
#define LIBJSONVAL_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Library version
extern const std::string VERSION;

/**
 * Validate a JSON string.
 *
 * @param text       JSON string to validate
 * @param error_msg  Output for detailed error message if validation fails
 * @return true if valid JSON, false otherwise
 */
bool validate_json(const std::string &text, std::string &error_msg);

/**
 * Print usage information.
 *
 * @param program_name executable name
 */
void print_help(const char *program_name);

// --- Schema registry ---
struct SchemaEntry {
  std::string id;
  std::string name;
  std::string description;
  std::string source; // local file path or http(s) URL
  std::string schemaVersion;
  std::vector<std::string> links;
};

// Initialize schema registry from a `schemas.json` file. Returns true on
// success.
bool init_schema_registry(const std::string &config_path, std::string &err);

// Get schema source content by id or source string (id|url|path). Returns true
// on success
bool get_schema_source(const std::string &id_or_source, std::string &out,
                       std::string &err);
// Resolve schema and its linked schemas into a map of id/uri -> content
bool resolve_schema_links(const std::string &id_or_source,
                          std::map<std::string, std::string> &out_map,
                          std::string &err);

// Return list of known schema ids
std::vector<std::string> list_schema_ids();

#endif // LIBJSONVAL_HPP
