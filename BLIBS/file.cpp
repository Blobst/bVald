// File Library for JLS
// File I/O and directory operations

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace jls_file {

// Forward declarations
struct Value;
using ValuePtr = std::shared_ptr<Value>;

// Read entire file as string
ValuePtr fn_read_file(const std::vector<ValuePtr> &args);

// Write string to file (overwrite)
ValuePtr fn_write_file(const std::vector<ValuePtr> &args);

// Append string to file
ValuePtr fn_append_file(const std::vector<ValuePtr> &args);

// Check if file exists
ValuePtr fn_file_exists(const std::vector<ValuePtr> &args);

// Delete file
ValuePtr fn_delete_file(const std::vector<ValuePtr> &args);

// Get file size in bytes
ValuePtr fn_file_size(const std::vector<ValuePtr> &args);

// List files in directory
ValuePtr fn_list_dir(const std::vector<ValuePtr> &args);

// Create directory
ValuePtr fn_mkdir(const std::vector<ValuePtr> &args);

// Check if path is a directory
ValuePtr fn_is_dir(const std::vector<ValuePtr> &args);

// Get current working directory
ValuePtr fn_getcwd(const std::vector<ValuePtr> &args);

// Change working directory
ValuePtr fn_chdir(const std::vector<ValuePtr> &args);

// Get all file library functions
std::map<std::string, ValuePtr> get_file_functions();

} // namespace jls_file
