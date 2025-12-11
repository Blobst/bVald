# libjsonval jq Integration

## Overview
The jq JSON query engine is now fully integrated into libjsonval, the core JSON library. This enables users to run jq filters directly through the libjsonval API.

## Public API

### 1. Basic Filter Execution
```cpp
bool run_jq_filter(const std::string &filter,
                   const std::string &json_in,
                   std::string &json_out,
                   std::string &err);
```

**Purpose**: Execute a jq filter and return the first output
**Parameters**:
- `filter`: jq filter expression (e.g., ".name", ".[] | .age")
- `json_in`: Input JSON string
- `json_out`: Output JSON string (first result)
- `err`: Error message if execution fails

**Example**:
```cpp
#include "libjsonval.hpp"

std::string output, error;
if (run_jq_filter(".name", R"({"name": "Alice", "age": 30})", output, error)) {
    std::cout << output << std::endl;  // prints: "Alice"
} else {
    std::cerr << "Error: " << error << std::endl;
}
```

### 2. Streaming (Multi-output) Execution
```cpp
bool run_jq_filter_streaming(const std::string &filter,
                             const std::string &json_in,
                             std::vector<std::string> &json_outputs,
                             std::string &err);
```

**Purpose**: Execute a jq filter and collect all outputs
**Parameters**:
- `filter`: jq filter expression
- `json_in`: Input JSON string
- `json_outputs`: Vector of output JSON strings
- `err`: Error message if execution fails

**Example**:
```cpp
std::vector<std::string> outputs;
std::string error;
if (run_jq_filter_streaming(".[]", "[1, 2, 3]", outputs, error)) {
    for (const auto &out : outputs) {
        std::cout << out << std::endl;  // prints: 1, 2, 3
    }
}
```

### 3. Custom Builtin Registration
```cpp
void register_jq_builtin(
    const std::string &name,
    const std::function<bool(const std::string &,
                             std::vector<std::string> &,
                             std::string &)> &fn);
```

**Purpose**: Register a custom jq builtin function
**Parameters**:
- `name`: Function name (used as builtin in jq filters)
- `fn`: Lambda/function that processes JSON input and produces outputs

**Function Signature**:
```cpp
bool custom_fn(const std::string &json_input,
               std::vector<std::string> &json_outputs,
               std::string &error) {
    // Process json_input
    // Push results to json_outputs
    // Return true on success, false on error
    return true;
}
```

**Example**:
```cpp
// Register a "double" function
register_jq_builtin("double", [](const std::string &json_in,
                                  std::vector<std::string> &outputs,
                                  std::string &err) -> bool {
    try {
        double val = std::stod(json_in);
        outputs.push_back(std::to_string(val * 2));
        return true;
    } catch (...) {
        err = "Invalid input";
        return false;
    }
});

// Use in jq filters
std::string result;
run_jq_filter("double", "5", result, error);
// result = "10"
```

## Architecture

```
libjsonval.hpp (public API)
    ↓
libjsonval.cpp (implementation)
    ↓
run_jq_filter() / run_jq_filter_streaming()
    ↓
jq::Engine (global instance)
    ├─ compile()
    ├─ run()
    ├─ run_streaming()
    └─ register_builtin()
    ↓
Lexer → Parser → Compiler → Executor → Builtins
```

## Supported jq Operations

### Basic Operations
- **Identity**: `.`
- **Field Access**: `.field`, `.nested.field`
- **Array/Object Indexing**: `.[0]`, `.[key]`
- **Array/Object Iteration**: `.[]`

### Operators
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Pipe**: `|` (composition)
- **Comma**: `,` (multiple outputs)

### Builtin Functions
- `keys` - Extract object keys
- `values` - Extract object/array values
- `type` - Get JSON type
- `length` - Get length
- `reverse` - Reverse array
- `sort` - Sort array
- `to_entries` - Convert object to key-value pairs
- `empty` - Return no output

## Integration with Other Components

### With libjsonval JSON DOM
```cpp
#include "libjsonval.hpp"

JsonValue obj;
std::string err;
parse_json_dom(R"({"name": "Bob", "age": 25})", obj, err);

// Can now use jq directly
std::string name;
run_jq_filter(".name", R"({"name": "Bob", "age": 25})", name, err);
```

### With JLS Scripting
The jq functions are also exposed through the JLS library system:
```jls
MANAGE jq
PRINT jq/run(".name", "{\"name\": \"Alice\"}")
PRINT jq/type("[1, 2, 3]")
PRINT jq/keys("{\"a\": 1, \"b\": 2}")
```

## Implementation Details

### Lazy Engine Initialization
The jq::Engine is created on-demand (lazy initialization) on the first call to `run_jq_filter()` or `run_jq_filter_streaming()`. This minimizes startup overhead.

### Builtin Adapter Layer
Custom builtins registered via `register_jq_builtin()` are adapted from string-based input/output to the internal JvValue-based API used by the jq engine. This provides a simple, intuitive interface while maintaining internal consistency.

### Thread Safety Considerations
The global jq::Engine instance is shared across calls. For multi-threaded applications, consider:
- Using thread-local storage for per-thread engines
- Synchronizing access with mutexes if shared

## Compilation

jq integration is automatically included when compiling libjsonval:
```bash
cd c:\Users\User\Desktop\projects\bVald
make
```

All jq sources are compiled as part of the main executable:
- jq_types.cpp - Type system
- jq_lexer.cpp - Tokenizer
- jq_parser.cpp - Parser
- jq_bytecode.cpp - Bytecode structures and utilities
- jq_compiler.cpp - Compiler
- jq_executor.cpp - Executor
- jq_builtins.cpp - Builtin registry
- jq_engine.cpp - High-level engine

## Future Enhancements

1. **Advanced Builtins**: map, select, group_by, reduce, min, max, etc.
2. **Recursive Descent**: `..` operator support
3. **Path Expressions**: getpath, setpath, delpaths
4. **Error Handling**: try-catch expressions
5. **String Interpolation**: `"Value: \(.)"` syntax
6. **User-Defined Functions**: Ability to define custom jq functions
7. **Performance Optimization**: Bytecode caching, JIT compilation

## Examples

### Extracting Nested Data
```cpp
std::string output, error;
std::string json = R"({
    "users": [
        {"name": "Alice", "age": 30},
        {"name": "Bob", "age": 25}
    ]
})";

run_jq_filter(".users[0].name", json, output, error);
// output = "Alice"
```

### Processing Arrays
```cpp
std::vector<std::string> outputs;
run_jq_filter_streaming(".[] | .age + 5", json, outputs, error);
// outputs = ["35", "30"]
```

### Using Custom Builtins
```cpp
// Register custom function
register_jq_builtin("magnitude", [](const std::string &json_in,
                                     std::vector<std::string> &outputs,
                                     std::string &err) -> bool {
    // Parse array [x, y]
    // Calculate sqrt(x^2 + y^2)
    // Push result
    return true;
});

// Use in filter
run_jq_filter("magnitude", "[3, 4]", output, error);
// output = "5"
```

## See Also
- [jq.hpp](include/jq.hpp) - jq engine public API
- [jls_library.cpp](include/jls_library.cpp) - JLS integration
- Official jq documentation: https://stedolan.github.io/jq/manual/
