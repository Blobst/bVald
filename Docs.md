# bVald - Complete Documentation

**bVald** is a unified JSON processing and scripting platform that combines JSON validation, the jq query engine, and a BASIC-like scripting language into a single, cohesive system. It provides both a C++ library (libjsonval) and an interactive scripting environment (JLS).

**Current Version:** 0.2.0  
**Status:** Production-Ready (Phase 5: Complete Integration)

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Installation & Building](#installation--building)
5. [API Reference](#api-reference)
6. [JLS Scripting Guide](#jls-scripting-guide)
7. [jq Query Guide](#jq-query-guide)
8. [Examples](#examples)
9. [Advanced Topics](#advanced-topics)
10. [Troubleshooting](#troubleshooting)

---

## Overview

### What is bVald?

bVald is a **unified JSON ecosystem** that integrates:

- **libjsonval** - A robust C++ JSON library with parsing, validation, and schema support
- **jq** - A full-featured JSON query language (ported to C++23)
- **JLS** - A BASIC-like scripting language for JSON manipulation
- **BSC** - Standard Collection (Math, IO, File operations)

### Key Capabilities

✅ **JSON Processing**
- Parse and validate JSON documents
- JSON Schema validation (draft-7 subLET)
- Pretty-print and tree visualization
- DOM manipulation

✅ **JSON Queries**
- Full jq filter support with streaming
- Field access, indexing, iteration
- Arithmetic and comparison operators
- 8+ builtin functions (keys, values, type, length, sort, reverse, etc.)

✅ **Scripting**
- BASIC-like JLS language
- Variables, arrays, objects
- Control flow (IF/ELSE, LOOPS)
- Function  and library management
- Interactive shell with command history

✅ **Standard Libraries**
- **Math**: sin, cos, tan, ln, log, exp, sqrt, pow, round, π, e
- **IO**: input, printno, print
- **File**: read_file, write_file, file_exists
- **jq**: Integrated query functions

✅ **Extensibility**
- Register custom builtin functions
- Custom library support with namespaces
- Bidirectional type conversion

---

## Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────┐
│                  bVald Ecosystem                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────────────────────────────────────────┐    │
│  │  User Applications (C++ or JLS Scripts)         │    │
│  └─────────────────┬───────────────────────────────┘    │
│                    │                                    │
│  ┌─────────────────▼───────────────────────────────┐    │
│  │         libjsonval Public API                   │    │
│  │  (JSON + jq + JLS Integration)                  │    │
│  └────┬──────────────────┬─────────────────────┬───┘    │
│       │                  │                     │        │
│   ┌───▼─────┐      ┌─────▼────┐         ┌─────▼──┐      │
│   │ JSON    │      │   jq     │         │  JLS   │      │
│   │ Layer   │      │ Engine   │         │ Shell  │      │
│   └────┬────┘      └─────┬────┘         └─────┬──┘      │
│        │                 │                    │         │
│        │          ┌──────┴───────┐            │         │
│        │          │              │            │         │
│   ┌────▼───┐  ┌───▼───┐  ┌──────▼───┐   ┌────▼──┐       │
│   │Validate│  │Compile│  │ Execute  │   │ Eval  │       │ 
│   │ Schema │  │(AST→  │  │(Stream)  │   │       │       │
│   │        │  │Byte)  │  │          │   │       │       │
│   └────────┘  └───────┘  └──────────┘   └───────┘       │
│                                                         │
│  ┌─────────────────────────────────────────────────┐    │
│  │  Standard Libraries (Math, IO, File, jq)        │    │
│  └─────────────────────────────────────────────────┘    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Execution Flow

```
C++ Code                JLS Script              Command Line
    │                        │                        │
    ├─► libjsonval.hpp       ├─► JLS Parser           ├─► bvald.exe
    │   (run_jq_filter)      │   (Evaluator)          │   (validator/shell)
    │                        │                        │
    └─► jq::Engine           └─► jq/Builtin     └─► JSON Validation
        (compile/run)             (via jls_library)      (schema check)
        │                         │
        ├─► Lexer                 └─► jq::Engine
        ├─► Parser
        ├─► Compiler
        └─► Executor
```

### Type System

All components share a unified **JvValue** type that bridges:

```cpp
JsonValue (libjsonval)  ←─────────────────┐
                                          │
                         ↓         ↕      ↓
                    JvValue (jq)    Value (JLS)
                         ↑         ↕      ↑
                                          │
libjsonval ◄──────────────────────────► JLS
    ↓                                    ↓
parse_json_dom()                   LOAD statement
validate_json()              JSON object/array construction
                                  jq/run() ```

---

## Core Components

### 1. libjsonval - JSON Library

**Purpose:** Core JSON parsing, validation, and DOM manipulation

**Capabilities:**
- Parse JSON to DOM tree (`JsonValue` struct)
- JSON validation (RFC 7159)
- JSON Schema validation (draft-7 subLET)
- Pretty-printing and tree visualization
- DLL/SO/DYLIB support for shared library use

**Key Types:**
```cpp
struct JsonValue {
    enum Type { T_NULL, T_BOOL, T_NUMBER, T_STRING, T_OBJECT, T_ARRAY };
    Type t;
    bool b;
    double n;
    std::string s;
    std::map<std::string, JsonValue> o;
    std::vector<JsonValue> a;
};

struct SchemaEntry {
    std::string id;
    std::string name;
    std::string source;
    std::string schemaVersion;
    std::vector<std::string> links;
};
```

**Functions:**
- `parse_json_dom(json_text, out, err)` - Parse JSON
- `validate_json(text, error_msg)` - Validate syntax
- `print_json_tree(val, prefix, is_last)` - Display tree
- `validate_json_with_schema(json_text, schema_text, err)` - Schema validation
- `init_schema_registry(path, err)` - Load schema registry
- `get_schema_source(id_or_source, out, err)` - Fetch schema
- `resolve_schema_links(id, out_map, err)` - Resolve dependencies
- `list_schema_ids()` - List available schemas

---

### 2. jq Engine - JSON Query Language

**Purpose:** Powerful, composable JSON filtering and transformation

**Architecture (5 Phases):**

#### Phase 1: Type System
- **JvValue** - Unified type with 6 variants (null, bool, number, string, array, object)
- **Bidirectional converters**: JsonValue ↔ JvValue ↔ Value (JLS)
- **Shared pointers** for memory safety

#### Phase 2: Lexer & Parser
- **Lexer** (jq_lexer.hpp/cpp): Tokenizes filters into 40+ token types
- **Parser** (jq_parser.hpp/cpp): Recursive descent parser with operator precedence
- **AST Generation**: Complete abstract syntax tree

**Supported Syntax:**
```jq
.                          # identity
.field                     # field access
.[0]                       # indexing
.[]                        # iteration
.[1:3]                     # slicing
.field | .nested | length  # pipes
.a, .b, .c                 # comma (multiple outputs)
.a + .b                    # arithmetic
.a == .b                   # comparison
(.a | length) + 1          # parentheses
[.a, .b]                   # array construction
{x: .a, y: .b}             # object construction
keys()                     # builtin functions
.[] | select(.age > 25)    # filters
```

#### Phase 3: Compiler & Bytecode
- **Compiler** (jq_compiler.hpp/cpp): AST → bytecode
- **Bytecode** (jq_bytecode.hpp/cpp): Canonical instruction format
- **Constant Pool**: String and number deduplication
- **Validation**: Semantic checking

**Instruction LET:**
```cpp
enum class OpCode {
    NOP,               // No operation
    LOAD_IDENTITY,     // Push input value
    GET_FIELD,         // Field access (.field)
    GET_INDEX_NUM,     // Array indexing (.[0])
    GET_INDEX_STR,     // Object indexing (.[key])
    ITERATE,           // Iteration (.[] or .a[])
    ADD_CONST,         // Arithmetic constant
    LENGTH,            // Length operation
    BUILTIN_      // Builtin function
};
```

#### Phase 4: Executor & Builtins
- **Executor** (jq_executor.hpp/cpp): Stream-based bytecode VM
- **Multi-output semantics**: Natural support for jq's streaming
- **Builtins** (jq_builtins.hpp/cpp): Extensible function registry

**8 Builtin Functions:**
- `keys` - Extract object keys (returns array)
- `values` - Extract object/array values
- `type` - Get JSON type ("null", "boolean", "number", "string", "array", "object")
- `length` - Get length (string char count, array/object size)
- `reverse` - Reverse array/string
- `sort` - Sort array
- `to_entries` - Convert {k:v} to [{key:k,value:v}]
- `empty` - Return no output

#### Phase 5: Integration
- **Engine** (jq_engine.hpp/cpp): High-level orchestrator
- **Streaming API**: `run_streaming()` for multiple outputs
- **JLS Integration**: Exposed via jls_library
- **libjsonval Integration**: `run_jq_filter()` in public API

**Example:**
```cpp
jq::Engine engine;
std::string output, error;

// Compile and run
engine.run(".name", R"({"name": "Alice"})", output, error);
// output = "Alice"

// Multi-output
std::vector<std::string> outputs;
engine.run_streaming(".[]", "[1, 2, 3]", outputs, error);
// outputs = ["1", "2", "3"]
```

---

### 3. JLS - JsonLambdaScript

**Purpose:** BASIC-like scripting language for JSON manipulation

**Language Features:**

#### Variables & Types
```jls
LET x = 5
LET name = "Alice"
LET arr = [1, 2, 3]
LET obj = {age: 30, city: "NYC"}
LET b = true
```

#### Operators
```jls
5 + 3              # Arithmetic: +, -, *, /, %
"hello" + "world"  # String concatenation
[1,2] + [3]        # Array concatenation
a == b             # Comparison: ==, !=, <, >, <=, >=
a AND b, a OR b    # Logic: AND, OR, NOT
```

#### Control Flow
```jls
IF condition THEN
  # statements
ELSE
  # statements
END IF

WHILE condition DO
  # statements
END WHILE

FOR i = 1 TO 10 DO
  # statements (i available)
END FOR
```

#### Arrays & Objects
```jls
LET arr = [1, 2, 3]
PRINT arr[0]           # Indexing
PRINT arr[1:3]         # Slicing (partial)
LET arr[0] = 10        # Assignment

LET obj = {name: "Bob", age: 25}
PRINT obj.name         # Field access
LET obj.age = 26       # Field assignment
```

#### Functions
```jls
FUNCTION add(a, b)
  RETURN a + b
END FUNCTION

add(2, 3)
```

#### I/O
```jls
INPUT var              # Read from stdin
PRINT expr             # Print to stdout
PRINT "x=" + x + " y=" + y  # String formatting
```

#### Standard Libraries
```jls
MANAGE math            # Load math library
MANAGE io              # Load IO library
MANAGE file            # Load file library
MANAGE jq              # Load jq library

# Use with namespace
math/sin(0)
io/printno(42)
file/read_file("data.txt")
jq/run(".name", json_string)
```

#### Math Functions
```jls
MANAGE math
math/sin(x)       # Sine (radians)
math/cos(x)       # Cosine (radians)
math/tan(x)       # Tangent (radians)
math/ln(x)        # Natural logarithm
math/log(x)       # Base-10 logarithm
math/exp(x)       # e^x
math/sqrt(x)      # Square root
math/pow(x, y)    # x^y
math/round(x)     # Round to integer
math/pi                # Constant π (3.14159...)
math/e                 # Constant e (2.71828...)
```

#### File Functions
```jls
MANAGE file
file/read_file("path.txt", content)
file/write_file("path.txt", content)
file/file_exists("path.txt", exists)
```

#### jq Functions
```jls
MANAGE jq
jq/run(filter, json)           # Execute jq filter
jq/keys(json)                  # Extract keys
jq/values(json)                # Extract values
jq/type(json)                  # Get type
jq/length(json)                # Get length
```

#### Comments
```jls
# This is a comment (full line)
LET x = 5  # Inline comment
```

---

### 4. Standard Libraries (BSC)

#### Math Library
```cpp
namespace bsc::math {
    double sin(double x);
    double cos(double x);
    double tan(double x);
    double ln(double x);
    double log(double x);
    double exp(double x);
    double sqrt(double x);
    double pow(double x, double y);
    double round(double x);
    const double PI = 3.14159265358979;
    const double E = 2.71828182845904;
}
```

#### IO Library
```cpp
namespace bsc::io {
    void printno(double value);
    void input(std::string &value);
    void print(const std::string &text);
}
```

#### File Library
```cpp
namespace bsc::file {
    bool read_file(const std::string &path, std::string &content);
    bool write_file(const std::string &path, const std::string &content);
    bool file_exists(const std::string &path);
}
```

---

## Installation & Building

### Prerequisites

- **Compiler**: clang++ (C++23 standard required)
- **OS**: Windows (primary), Linux, macOS
- **Dependencies**: Standard C++ library (no external deps)

### Building

```bash
# Clone or download bVald
cd c:\Users\User\Desktop\projects\bVald

# Build main executable
make                    # Creates build/bvald.exe (907.2 kB)

# Build shared library (DLL/SO/DYLIB)
make shared-NT          # Creates lib/libjsonval.dll (652.8 kB)
make shared-Linux       # Creates lib/libjsonval.so
make shared-Darwin      # Creates lib/libjsonval.dylib ngl i dont know why mac is called Darwin

# Clean build artifacts
make clean
```

### Project Structure

```
bVald/
├── src/jq/                    # jq implementation
│   ├── jq_types.hpp/cpp       # JvValue type system
│   ├── jq_lexer.hpp/cpp       # Tokenizer
│   ├── jq_parser.hpp/cpp      # Parser with AST
│   ├── jq_bytecode.hpp/cpp    # Bytecode structures
│   ├── jq_compiler.hpp/cpp    # Compiler
│   ├── jq_executor.hpp/cpp    # Executor (VM)
│   ├── jq_builtins.hpp/cpp    # Builtin registry
│   └── jq_engine.hpp/cpp      # Engine orchestrator
├── include/
│   ├── libjsonval.hpp/cpp     # JSON library (public API)
│   ├── jq.hpp                 # jq public API
│   ├── jls.hpp/cpp            # JLS core
│   ├── jls_shell.hpp/cpp      # Interactive shell
│   ├── jls_library.hpp/cpp    # Library system
│   └── libjsonval.cpp         # Implementation
├── build/
│   └── bvald.exe              # Main executable
├── lib/
│   ├── libjsonval.dll         # Shared library (Windows)
│   └── libjsonval.lib         # Import library
├── Makefile                   # Build configuration
├── main.cpp                   # Entry point
└── README.md                  # Quick start
```

---

## API Reference

### C++ API (libjsonval.hpp)

#### JSON Functions

```cpp
// Parse JSON to DOM
bool parse_json_dom(const std::string &json_text, JsonValue &out,
                    std::string &err);

// Validate JSON syntax
bool validate_json(const std::string &text, std::string &error_msg);

// Validate against schema
bool validate_json_with_schema(const std::string &json_text,
                               const std::string &schema_text,
                               std::string &err);

// Print tree structure
void print_json_tree(const JsonValue &val,
                     const std::string &prefix = "",
                     bool is_last = true);

// Schema management
bool init_schema_registry(const std::string &config_path, std::string &err);
bool get_schema_source(const std::string &id_or_source, std::string &out,
                       std::string &err);
bool resolve_schema_links(const std::string &id_or_source,
                          std::map<std::string, std::string> &out_map,
                          std::string &err);
std::vector<std::string> list_schema_ids();
```

#### jq Functions (High-level)

```cpp
// Single output
bool run_jq_filter(const std::string &filter,
                   const std::string &json_in,
                   std::string &json_out,
                   std::string &err);

// Multiple outputs (streaming)
bool run_jq_filter_streaming(const std::string &filter,
                             const std::string &json_in,
                             std::vector<std::string> &json_outputs,
                             std::string &err);

// Register custom builtin
void register_jq_builtin(
    const std::string &name,
    const std::function<bool(const std::string &,
                             std::vector<std::string> &,
                             std::string &)> &fn);
```

#### jq Components (Low-level)

```cpp
// Lexer
JqLexer lexer(filter_string);
auto token = lexer.next_token();

// Parser
JqParser parser(lexer);
auto ast = parser.parse();

// Compiler
JqCompiler compiler;
auto program = compiler.compile(ast);

// Executor
JqExecutor executor;
std::vector<JqValuePtr> outputs;
executor.execute(*program, input_jv_value, outputs, error);

// Builtins
std::vector<JqValuePtr> result;
JqBuiltins::builtin("sort", input, result, error);

// Engine
JqEngine engine;
engine.compile(".name", error);
engine.run(".name", json_in, json_out, error);
engine.run_streaming(".[]", json_in, outputs, error);
JqEngine::register_builtin("custom", fn);

// Print program (debugging)
jq::print_program(*program, std::cout);
```

### Command Line Interface

```bash
# JSON validation
bvald.exe file.json
bvald.exe file.json -s schema_id
bvald.exe file.json --use-schema

# Schema management
bvald.exe -s schema_id          # Fetch and display schema
bvald.exe --schema schema_id    # Same as above

# Interactive JLS shell
bvald.exe -S
bvald.exe --shell

# Help
bvald.exe -h
bvald.exe --version
```

---

## JLS Scripting Guide

### Basic Script

```jls
# Script: process_users.jls
# NOTE: scripts aren't run directly as files like e.g. bvald process_users.jils won't work

MANAGE jq
MANAGE math

LET json = "{\"users\": [{\"name\": \"Alice\", \"age\": 30}, {\"name\": \"Bob\", \"age\": 25}]}"


# Calculate
LET angle = 1.5708  # π/2
LET result = math/sin(angle)
```

### JSON Manipulation

```jls
MANAGE jq

LET data = "{\"items\": [5, 2, 8, 1, 9]}"

# Get keys
LET keys = jq/run("keys", data)

# Check type
LET type = jq/run("type", data)
```


### Interactive Shell

```
bvald.exe -S

jls> LET x = 42
jls> PRINT x
42
jls> MANAGE math
jls> PRINT math/sin(0)
0
jls> MANAGE jq
jls> LET json = "{\"name\": \"Alice\"}"
jls> PRINT jq/run(".name", json)
"Alice"
jls> exit
```

---

## jq Query Guide

### Basic Queries

```jq
# Input: {"name": "Alice", "age": 30}

.name                  # "Alice"
.age                   # 30
.                      # {"name": "Alice", "age": 30} (identity)
.missing               # null (missing field)
```

### Arrays

```jq
# Input: [1, 2, 3, 4, 5]

.[]                    # 1, 2, 3, 4, 5 (iterate)
.[0]                   # 1 (first element)
.[2]                   # 3 (index)
.[-1]                  # 5 (last element)
.[1:3]                 # [2, 3] (slice)
length                 # 5
reverse                # [5, 4, 3, 2, 1]
sort                   # [1, 2, 3, 4, 5] (already sorted)
```

### Objects

```jq
# Input: {"a": 1, "b": 2, "c": 3}

.a, .b, .c             # 1, 2, 3 (multiple outputs)
keys                   # ["a", "b", "c"]
values                 # 1, 2, 3
to_entries             # [{key: "a", value: 1}, ...]
type                   # "object"
length                 # 3
```

### Pipes (Composition)

```jq
# Input: {"items": [1, 2, 3]}

.items | length        # 3
.items | reverse       # [3, 2, 1]
.items | .[]           # 1, 2, 3
.items | .[0]          # 1

# Multi-step
.items | reverse | .[0]  # 3
```

### Arithmetic

```jq
# Input: {"a": 10, "b": 3}

.a + .b                # 13
.a - .b                # 7
.a * .b                # 30
.a / .b                # 3.33...
.a % .b                # 1
(.a + .b) * 2          # 26
```

### Comparison

```jq
# Input: {"x": 5, "y": 3}

.x == .y               # false
.x != .y               # true
.x > .y                # true
.x < .y                # false
.x >= .y               # true
.x <= .y               # false
```

### Nested Access

```jq
# Input: {"user": {"name": "Alice", "profile": {"age": 30}}}

.user.name             # "Alice"
.user.profile.age      # 30
.user.profile          # {"age": 30}
```

### Complex Examples

```jq
# Input: {"users": [{"name": "Alice", "age": 30}, {"name": "Bob", "age": 25}]}

.users[]               # Iterate users
.users[] | .name       # "Alice", "Bob"
.users | length        # 2
.users | reverse | .[0].name  # "Bob"
```

---

## Examples

### Example 1: JSON Validation in C++

```cpp
#include "include/libjsonval.hpp"
#include <iostream>

int main() {
    std::string json = R"({
        "name": "Alice",
        "age": 30,
        "email": "alice@example.com"
    })";
    
    std::string error;
    
    // Validate JSON syntax
    if (validate_json(json, error)) {
        std::cout << "Valid JSON!" << std::endl;
    } else {
        std::cout << "Invalid: " << error << std::endl;
    }
    
    // Parse to DOM
    JsonValue dom;
    if (parse_json_dom(json, dom, error)) {
        print_json_tree(dom);
    }
    
    return 0;
}
```

### Example 2: jq Filtering in C++

```cpp
#include "include/libjsonval.hpp"
#include <iostream>
#include <vector>

int main() {
    std::string json = R"({
        "products": [
            {"name": "Laptop", "price": 1000},
            {"name": "Phone", "price": 500},
            {"name": "Tablet", "price": 300}
        ]
    })";
    
    std::string output, error;
    
    // Single output
    if (run_jq_filter(".products[0].name", json, output, error)) {
        std::cout << "First product: " << output << std::endl;
    }
    
    // Multiple outputs (streaming)
    std::vector<std::string> outputs;
    if (run_jq_filter_streaming(".products[] | .name", json, outputs, error)) {
        std::cout << "All products:" << std::endl;
        for (const auto &name : outputs) {
            std::cout << "  - " << name << std::endl;
        }
    }
    
    // Register custom function
    register_jq_builtin("times10", [](const std::string &json_in,
                                       std::vector<std::string> &outputs,
                                       std::string &err) -> bool {
        try {
            double val = std::stod(json_in);
            outputs.push_back(std::to_string(val * 10));
            return true;
        } catch (...) {
            err = "Not a number";
            return false;
        }
    });
    
    if (run_jq_filter("times10", "5", output, error)) {
        std::cout << "5 * 10 = " << output << std::endl;
    }
    
    return 0;
}
```

### Example 3: JLS Scripting

```jls
# Script: data_analysis.jls
MANAGE jq
MANAGE math
MANAGE file

# Read data
file/read_file("data.json", json_data)

# Extract and analyze
LET records = jq/run(".records", json_data)
LET count = jq/run(".records | length", json_data)
LET types = jq/run(".records[] | type", json_data)

PRINT "Total records: " + count
PRINT "Record types: " + types

# Process with math
LET total = 0
FOR i = 1 TO count DO
  LET val = jq/run(".records[" + i + "]", json_data)
  LET total = total + val
END FOR

LET average = total / count
PRINT "Average: " + average
PRINT "Rounded: " + math/round(average)

# Save results
LET result = "{\"average\": " + average + ", \"count\": " + count + "}"
file/write_file("results.json", result)
```

### Example 4: Low-level jq Components

```cpp
#include "include/libjsonval.hpp"
#include <iostream>

int main() {
    std::string filter = ".users | length";
    std::string json = R"({"users": [1, 2, 3, 4, 5]})";
    
    // 1. Lexer: Tokenize
    std::cout << "=== Lexer ===" << std::endl;
    JqLexer lexer(filter);
    while (true) {
        auto tok = lexer.next_token();
        if (tok.type == jq::TokenType::TOK_EOF) break;
        std::cout << "Token: " << static_cast<int>(tok.type);
        if (!tok.text.empty()) std::cout << " \"" << tok.text << "\"";
        std::cout << std::endl;
    }
    
    // 2. Parser: Generate AST
    std::cout << "\n=== Parser ===" << std::endl;
    JqLexer lexer2(filter);
    JqParser parser(lexer2);
    auto ast = parser.parse();
    if (ast) {
        std::cout << "AST created (type: " << static_cast<int>(ast->type) << ")" << std::endl;
    }
    
    // 3. Compiler: Generate bytecode
    std::cout << "\n=== Compiler ===" << std::endl;
    JqCompiler compiler;
    auto program = compiler.compile(ast);
    if (program) {
        std::cout << "Bytecode generated" << std::endl;
        jq::print_program(*program, std::cout);
    }
    
    // 4. Executor: Run bytecode
    std::cout << "\n=== Executor ===" << std::endl;
    auto input = JqValue::from_string(json);
    JqExecutor executor;
    std::vector<JqValuePtr> outputs;
    std::string error;
    
    if (executor.execute(*program, input, outputs, error)) {
        std::cout << "Execution successful" << std::endl;
        for (size_t i = 0; i < outputs.size(); ++i) {
            std::cout << "Output[" << i << "]: " << outputs[i]->to_string() << std::endl;
        }
    }
    
    return 0;
}
```

---

## Advanced Topics

### Custom Builtin Functions

#### From C++

```cpp
// String-based interface (simple)
register_jq_builtin("myfunction", 
    [](const std::string &json_in,
       std::vector<std::string> &outputs,
       std::string &err) -> bool {
        // Parse input, process, populate outputs
        outputs.push_back(result);
        return true;
    }
);

// JvValue-based interface (advanced)
JqEngine::register_builtin("myfunction",
    [](const JqValuePtr &input,
       std::vector<JqValuePtr> &outputs,
       std::string &err) -> bool {
        // Direct access to JvValue
        if (input->type == JqValue::TYPE_NUMBER) {
            auto result = std::make_shared<JqValue>();
            result->type = JqValue::TYPE_NUMBER;
            result->num = input->num * 2;
            outputs.push_back(result);
        }
        return true;
    }
);
```

#### Usage in jq Filters

```cpp
// After registering "double" function
std::string output, error;
run_jq_filter("double", "21", output, error);
// output = "42"

run_jq_filter(".[] | double", "[5, 10, 15]", outputs, error);
// outputs = ["10", "20", "30"]
```

### Streaming and Multi-output

```cpp
// jq naturally produces multiple outputs
std::vector<std::string> outputs;
std::string error;

run_jq_filter_streaming(".[]", "[1, 2, 3]", outputs, error);
// outputs.size() = 3
// outputs[0] = "1"
// outputs[1] = "2"
// outputs[2] = "3"

run_jq_filter_streaming(".[], .[]", "[1, 2]", outputs, error);
// outputs.size() = 4
// outputs = ["1", "2", "1", "2"]
```

### Type Conversions

The unified type system enables seamless conversion:

```cpp
// JsonValue (libjsonval) → JvValue (jq)
JsonValue jv = ...;
JvValuePtr jv_val = jq::JvValue::from_json_value(jv);

// JvValue (jq) → JsonValue (libjsonval)
JsonValue json_val = jq::to_json_value(jv_val);

// Value (JLS) → JvValue (jq)
JvValuePtr jq_val = jq::JvValue::from_jls_value(ls_val);

// JvValue (jq) → Value (JLS)
Value ls_val = jq::to_jls_value(jq_val);
```

### Schema Registry

```cpp
// Initialize from config file
std::string error;
init_schema_registry("schemas.json", error);

// Get schema by ID
std::string schema_content;
get_schema_source("schema_id", schema_content, error);

// Resolve schema and dependencies
std::map<std::string, std::string> resolved;
resolve_schema_links("schema_id", resolved, error);

// List available schemas
auto ids = list_schema_ids();
for (const auto &id : ids) {
    std::cout << id << std::endl;
}
```

### Performance Considerations

1. **Lazy Compilation**: Filters are compiled once, can be reused
2. **Streaming Execution**: Multiple outputs don't require collection
3. **Constant Pooling**: String/number deduplication in bytecode
4. **Shared Pointers**: Smart memory management for JvValue

---

## Troubleshooting

### Common Issues

**Issue: Parse error in jq filter**
```
Error: Unexpected token at position X
```
- Check filter syntax (common: missing quotes around strings)
- Verify field names match actual JSON structure
- Test with simpler filter first

**Issue: JSON validation fails**
```
Error: Invalid JSON: Unexpected character 'x' at line 1 col 10
```
- Use proper JSON syntax (double quotes, trailing commas not allowed)
- Validate with online JSON validator first
- Check for encoding issues (ensure UTF-8)

**Issue: Schema validation fails**
```
Error: Failed to load schema
```
- Verify schema file exists (if local path)
- Check schema registry configuration
- Ensure schema version is supported (draft-7 subLET)

**Issue: JLS compilation error**
```
Error: Undefined variable 'x'
```
- Initialize variable before use: `LET x = value`
- Check variable scope (variables are global in JLS)
- Verify correct spelling

**Issue: DLL/SO not found**
```
Error: Cannot find libjsonval.dll
```
- Ensure shared library is built: `make shared-Windows`
- Add lib directory to PATH or LD_LIBRARY_PATH
- Use absolute path if needed

### Debug Output

Enable verbose output for debugging:

```cpp
// In C++ code
jq::print_program(*program, std::cout);  // Show compiled bytecode

// From command line
bvald.exe -h                              // Show help
bvald.exe --version                       # Show version
```

### Performance Profiling

For slow operations:

1. **Compile once, run multiple times:**
   ```cpp
   JqEngine engine;
   engine.compile(filter, err);
   for (const auto &input : inputs) {
       std::string output;
       engine.run(filter, input, output, err);
   }
   ```

2. **Use streaming for large arrays:**
   ```cpp
   std::vector<std::string> outputs;
   run_jq_filter_streaming(".[]", large_array, outputs, err);
   // Process outputs on-the-fly rather than buffering
   ```

3. **Profile bytecode:**
   ```cpp
   jq::print_program(*program);  // Check instruction count
   ```

---

## Summary

**bVald** provides:

✅ **Complete JSON ecosystem** - Parsing, validation, schemas, queries
✅ **Full jq implementation** - All core features with streaming support
✅ **Scripting language** - JLS for automation and manipulation
✅ **Standard libraries** - Math, IO, File, and more
✅ **Multiple APIs** - C++, JLS, command-line
✅ **Extensibility** - Custom builtins and libraries
✅ **Production ready** - Fully integrated, well-tested components

Whether you're processing JSON in C++, scripting with JLS, or validating against schemas, **bVald** provides a unified, powerful platform for all your JSON needs.

---

## Getting Help

- **Quick Start**: See README.md
- **Examples**: Check example_jq_api.cpp
- **API Details**: Review include/libjsonval.hpp
- **Source Code**: Explore src/jq/ for implementation

**Version**: 0.2.1  
**Last Updated**: December 2025  
**License**: See LICENSE.txt
