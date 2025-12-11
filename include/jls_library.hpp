#ifndef JLS_LIBRARY_HPP
#define JLS_LIBRARY_HPP

#include "jls.hpp"
#include <map>
#include <set>
#include <string>

namespace jls {

// Library loader class
class LibraryLoader {
public:
  // Load a library by name (math, io, file, jq)
  // Returns true on success, false if library not found
  static bool load_library(const std::string &lib_name, EnvironmentPtr env);

  // Get list of available libraries
  static std::vector<std::string> get_available_libraries();

  // Register a user-provided library of native functions
  static void register_custom_library(
      const std::string &lib_name,
      const std::map<std::string, NativeFunctionPtr> &functions);

  // List functions exposed by a library (built-in or custom)
  static std::vector<std::string>
  get_library_functions(const std::string &lib_name);

  // Check if a name corresponds to a known library (built-in or custom)
  static bool is_library_name(const std::string &lib_name);

private:
  // Internal load functions
  static bool load_math_library(EnvironmentPtr env);
  static bool load_io_library(EnvironmentPtr env);
  static bool load_file_library(EnvironmentPtr env);
  static bool load_jq_library(EnvironmentPtr env);
};

} // namespace jls

#endif // JLS_LIBRARY_HPP
