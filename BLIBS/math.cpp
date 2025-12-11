// Math Library for JLS
// Extended math functions beyond the core BSC

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace jls_math {

// Forward declarations
struct Value;
using ValuePtr = std::shared_ptr<Value>;

// Sin function (in radians)
ValuePtr fn_sin(const std::vector<ValuePtr> &args);

// Cos function (in radians)
ValuePtr fn_cos(const std::vector<ValuePtr> &args);

// Tan function (in radians)
ValuePtr fn_tan(const std::vector<ValuePtr> &args);

// Square root (same as SQRT but included for completeness)
ValuePtr fn_sqrt(const std::vector<ValuePtr> &args);

// Natural logarithm
ValuePtr fn_ln(const std::vector<ValuePtr> &args);

// Base-10 logarithm
ValuePtr fn_log(const std::vector<ValuePtr> &args);

// Exponential function (e^x)
ValuePtr fn_exp(const std::vector<ValuePtr> &args);

// Round to nearest integer
ValuePtr fn_round(const std::vector<ValuePtr> &args);

// Absolute value
ValuePtr fn_abs(const std::vector<ValuePtr> &args);

// Degrees to radians
ValuePtr fn_deg2rad(const std::vector<ValuePtr> &args);

// Radians to degrees
ValuePtr fn_rad2deg(const std::vector<ValuePtr> &args);

// Pi constant
ValuePtr fn_pi(const std::vector<ValuePtr> &args);

// E constant
ValuePtr fn_e(const std::vector<ValuePtr> &args);

// Get all math library functions
// Returns a map of function names to function pointers
std::map<std::string, ValuePtr> get_math_functions();

} // namespace jls_math
