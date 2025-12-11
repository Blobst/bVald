#ifndef JLS_HPP
#define JLS_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// JsonLambdaScript (JLS) - A lambda-based scripting language for JSON
// manipulation Syntax is based on Lisp with lambda functions and functional
// programming

namespace jls {

// ================= Value Types =================
enum class ValueType {
  NIL,
  BOOLEAN,
  INTEGER,
  FLOAT,
  STRING,
  FUNCTION,
  LAMBDA,
  LIST,
  MAP
};

// Forward declarations
struct Value;
struct Environment;
struct Function;

using ValuePtr = std::shared_ptr<Value>;
using EnvironmentPtr = std::shared_ptr<Environment>;
using FunctionPtr = std::shared_ptr<Function>;
using NativeFunctionPtr =
    std::function<ValuePtr(const std::vector<ValuePtr> &)>;

// ================= Value Representation =================
struct Value {
  ValueType type;
  bool b = false;
  long long i = 0;
  double f = 0.0;
  std::string s;
  std::vector<ValuePtr> list;
  std::map<std::string, ValuePtr> map;
  FunctionPtr func;
  NativeFunctionPtr native_func;

  Value() : type(ValueType::NIL) {}
  explicit Value(bool v) : type(ValueType::BOOLEAN), b(v) {}
  explicit Value(long long v) : type(ValueType::INTEGER), i(v) {}
  explicit Value(int v) : type(ValueType::INTEGER), i(v) {}
  explicit Value(double v) : type(ValueType::FLOAT), f(v) {}
  explicit Value(const std::string &v) : type(ValueType::STRING), s(v) {}
  explicit Value(const char *v) : type(ValueType::STRING), s(v) {}
};

// ================= Tokens =================
enum class TokenType {
  // Literals
  INTEGER,
  FLOAT,
  STRING,
  IDENTIFIER,

  // Keywords
  PRINT,
  LET,
  IF,
  THEN,
  ELSE,
  END,
  FOR,
  TO,
  STEP,
  NEXT,
  WHILE,
  DO,
  FUNCTION,
  CALL,
  RETURN,
  TRUE,
  FALSE,
  NIL,
  AND,
  OR,
  NOT,

  // Delimiters
  LPAREN,   // (
  RPAREN,   // )
  LBRACKET, // [
  RBRACKET, // ]
  COMMA,    // ,
  NEWLINE,  // \n

  // Operators
  PLUS,    // +
  MINUS,   // -
  STAR,    // *
  SLASH,   // /
  PERCENT, // %
  CARET,   // ^
  EQUALS,  // =
  LT,      // <
  GT,      // >
  LTE,     // <=
  GTE,     // >=
  NEQ,     // <>
  EQ,      // ==

  // End of file
  EOF_TOKEN,

  // Error
  ERROR
};

struct Token {
  TokenType type;
  std::string value;
  size_t line;
  size_t column;

  Token() : type(TokenType::ERROR), line(0), column(0) {}
  Token(TokenType t, const std::string &v, size_t l, size_t c)
      : type(t), value(v), line(l), column(c) {}
};

// ================= Lexer =================
class Lexer {
public:
  explicit Lexer(const std::string &source);
  Token next_token();
  std::vector<Token> tokenize();

private:
  std::string source;
  size_t position = 0;
  size_t line = 1;
  size_t column = 1;

  char current_char();
  char peek_char(size_t offset = 1);
  void advance();
  void skip_whitespace();
  void skip_comment();
  Token read_number();
  Token read_string();
  Token read_identifier();
};

// ================= AST Node Types =================
enum class NodeType {
  LITERAL,
  IDENTIFIER,
  BINARY_OP,
  UNARY_OP,
  PRINT,
  LET,
  ASSIGNMENT,
  IF_STMT,
  FOR_LOOP,
  WHILE_LOOP,
  FUNCTION_DEF,
  FUNCTION_CALL,
  RETURN_STMT,
  BLOCK
};

struct ASTNode {
  NodeType type;
  ValuePtr literal_value;
  std::string identifier_name;
  std::string op; // For binary/unary operations
  std::vector<std::shared_ptr<ASTNode>> children;
  std::shared_ptr<ASTNode> condition;
  std::shared_ptr<ASTNode> then_branch;
  std::shared_ptr<ASTNode> else_branch;

  ASTNode() : type(NodeType::LITERAL) {}
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

// ================= Parser =================
class Parser {
public:
  explicit Parser(const std::vector<Token> &tokens);
  ASTNodePtr parse();
  std::string error_message() const { return error_msg; }

private:
  std::vector<Token> tokens;
  size_t position = 0;
  std::string error_msg;

  Token current_token();
  Token peek_token(size_t offset = 1);
  void advance();
  bool expect(TokenType type);

  ASTNodePtr parse_statement();
  ASTNodePtr parse_print();
  ASTNodePtr parse_let();
  ASTNodePtr parse_assignment();
  ASTNodePtr parse_if();
  ASTNodePtr parse_for();
  ASTNodePtr parse_while();
  ASTNodePtr parse_function();
  ASTNodePtr parse_expression();
  ASTNodePtr parse_comparison();
  ASTNodePtr parse_term();
  ASTNodePtr parse_factor();
  ASTNodePtr parse_primary();
};

// ================= Bytecode =================
enum class OpCode : unsigned char {
  // Stack operations
  PUSH_NIL = 0,
  PUSH_TRUE = 1,
  PUSH_FALSE = 2,
  PUSH_INT = 3,
  PUSH_FLOAT = 4,
  PUSH_STRING = 5,

  // Variables
  LOAD_VAR = 10,
  STORE_VAR = 11,

  // Function calls
  CALL = 20,
  RETURN = 21,

  // Arithmetic
  ADD = 30,
  SUB = 31,
  MUL = 32,
  DIV = 33,
  MOD = 34,
  POW = 35,

  // Comparison
  EQ = 40,
  NEQ = 41,
  LT = 42,
  LTE = 43,
  GT = 44,
  GTE = 45,

  // Logical
  AND = 50,
  OR = 51,
  NOT = 52,

  // Control flow
  JMP = 60,
  JMP_FALSE = 61,
  JMP_TRUE = 62,

  // List operations
  LIST_NEW = 70,
  LIST_PUSH = 71,
  LIST_GET = 72,
  LIST_SET = 73,

  // Map operations
  MAP_NEW = 80,
  MAP_SET = 81,
  MAP_GET = 82,

  // Halt
  HALT = 255
};

struct Instruction {
  OpCode op;
  std::vector<unsigned char> args;

  Instruction(OpCode o) : op(o) {}
};

// ================= Bytecode Compiler =================
class Compiler {
public:
  std::vector<Instruction> compile(const ASTNodePtr &ast);
  std::string error_message() const { return error_msg; }

private:
  std::vector<Instruction> bytecode;
  std::map<std::string, int> variables;
  std::string error_msg;
  int var_count = 0;

  void compile_node(const ASTNodePtr &node);
  void emit(OpCode op);
  void emit_int(long long val);
  void emit_float(double val);
  void emit_string(const std::string &val);
};

// ================= Bytecode Interpreter/VM =================
class VM {
public:
  VM();
  bool execute(const std::vector<Instruction> &bytecode);
  ValuePtr get_result() const { return result; }
  std::string error_message() const { return error_msg; }

private:
  std::vector<ValuePtr> stack;
  std::map<std::string, ValuePtr> variables;
  ValuePtr result;
  std::string error_msg;

  void push(const ValuePtr &val);
  ValuePtr pop();
  ValuePtr peek();
};

// ================= Bvald Standard Collection (BSC) =================
class BSC {
public:
  static void register_functions(EnvironmentPtr env);

private:
  // Math functions
  static ValuePtr fn_add(const std::vector<ValuePtr> &args);
  static ValuePtr fn_sub(const std::vector<ValuePtr> &args);
  static ValuePtr fn_mul(const std::vector<ValuePtr> &args);
  static ValuePtr fn_div(const std::vector<ValuePtr> &args);
  static ValuePtr fn_mod(const std::vector<ValuePtr> &args);
  static ValuePtr fn_pow(const std::vector<ValuePtr> &args);
  static ValuePtr fn_sqrt(const std::vector<ValuePtr> &args);
  static ValuePtr fn_floor(const std::vector<ValuePtr> &args);
  static ValuePtr fn_ceil(const std::vector<ValuePtr> &args);
  static ValuePtr fn_abs(const std::vector<ValuePtr> &args);
  static ValuePtr fn_min(const std::vector<ValuePtr> &args);
  static ValuePtr fn_max(const std::vector<ValuePtr> &args);

  // String functions
  static ValuePtr fn_string(const std::vector<ValuePtr> &args);
  static ValuePtr fn_length(const std::vector<ValuePtr> &args);
  static ValuePtr fn_concat(const std::vector<ValuePtr> &args);
  static ValuePtr fn_substring(const std::vector<ValuePtr> &args);

  // List functions
  static ValuePtr fn_list(const std::vector<ValuePtr> &args);
  static ValuePtr fn_head(const std::vector<ValuePtr> &args);
  static ValuePtr fn_tail(const std::vector<ValuePtr> &args);
  static ValuePtr fn_nth(const std::vector<ValuePtr> &args);

  // Control flow
  static ValuePtr fn_if(const std::vector<ValuePtr> &args);
  static ValuePtr fn_cond(const std::vector<ValuePtr> &args);

  // I/O
  static ValuePtr fn_print(const std::vector<ValuePtr> &args);
  static ValuePtr fn_input(const std::vector<ValuePtr> &args);

  // Utility
  static ValuePtr fn_random(const std::vector<ValuePtr> &args);
  static ValuePtr fn_type(const std::vector<ValuePtr> &args);
  static ValuePtr fn_int(const std::vector<ValuePtr> &args);
  static ValuePtr fn_float(const std::vector<ValuePtr> &args);
};

// ================= Environment =================
struct Function {
  std::vector<std::string> params;
  ASTNodePtr body;
  EnvironmentPtr closure;
};

struct Environment {
  std::map<std::string, ValuePtr> variables;
  EnvironmentPtr parent;

  explicit Environment(EnvironmentPtr p = nullptr) : parent(p) {}

  ValuePtr get(const std::string &name);
  void set(const std::string &name, const ValuePtr &value);
  bool exists(const std::string &name) const;
};

// ================= Evaluator =================
class Evaluator {
public:
  Evaluator();
  ValuePtr eval(const ASTNodePtr &node, EnvironmentPtr env = nullptr);
  std::string error_message() const { return error_msg; }
  EnvironmentPtr get_global_env() { return global_env; }

private:
  EnvironmentPtr global_env;
  std::string error_msg;

  ValuePtr eval_node(const ASTNodePtr &node, EnvironmentPtr env);
  ValuePtr call_function(const ValuePtr &func,
                         const std::vector<ValuePtr> &args, EnvironmentPtr env);
};

} // namespace jls

#endif // JLS_HPP
