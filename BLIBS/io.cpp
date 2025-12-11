// I/O Library for JLS
// Extended input/output functions

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace jls_io {

// Forward declarations
struct Value;
using ValuePtr = std::shared_ptr<Value>;

// Print with no newline (unlike PRINT which adds newline)
ValuePtr fn_printno(const std::vector<ValuePtr> &args);

// Print to stderr
ValuePtr fn_printerr(const std::vector<ValuePtr> &args);

// Get a line of input from user
ValuePtr fn_getline(const std::vector<ValuePtr> &args);

// Get a single character input
ValuePtr fn_getchar(const std::vector<ValuePtr> &args);

// Print formatted value with prefix
ValuePtr fn_printf(const std::vector<ValuePtr> &args);

// Clear the console/terminal
ValuePtr fn_cls(const std::vector<ValuePtr> &args);

// Pause execution and wait for user input
ValuePtr fn_pause(const std::vector<ValuePtr> &args);

// Beep/bell sound
ValuePtr fn_beep(const std::vector<ValuePtr> &args);

// Get all I/O library functions
std::map<std::string, ValuePtr> get_io_functions();

} // namespace jls_io
