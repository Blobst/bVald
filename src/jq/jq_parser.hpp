#ifndef JQ_PARSER_HPP
#define JQ_PARSER_HPP

#include "jq_lexer.hpp"
#include "jq_types.hpp"
#include <memory>
#include <vector>

namespace jq {

// AST Node types for jq expressions
enum class NodeType {
  // Literals
  LITERAL,

  // Identity and field access
  IDENTITY,  // .
  FIELD,     // .foo
  INDEX,     // .[0] or .["key"]
  SLICE,     // .[1:3]
  ITERATOR,  // .[]
  RECURSIVE, // ..

  // Operations
  PIPE,      // |
  COMMA,     // ,
  BINARY_OP, // +, -, *, /, ==, !=, <, >, etc.
  UNARY_OP,  // not, -

  // Functions
  FUNCTION_CALL, // map(), select(), etc.

  // Constructors
  ARRAY,  // []
  OBJECT, // {}

  // Conditionals
  CONDITIONAL, // if-then-else
  TRY,         // try-catch

  // Alternative operator
  ALTERNATIVE // //
};

class ASTNode;
using ASTNodePtr = std::shared_ptr<ASTNode>;

class ASTNode {
public:
  NodeType type;

  // For literals
  JvValuePtr literal;

  // For identifiers/fields
  std::string name;

  // For operators
  std::string op;

  // Children nodes
  std::vector<ASTNodePtr> children;

  // For conditionals
  ASTNodePtr condition;
  ASTNodePtr then_branch;
  ASTNodePtr else_branch;

  ASTNode(NodeType t = NodeType::IDENTITY) : type(t) {}

  static ASTNodePtr make_literal(const JvValuePtr &val);
  static ASTNodePtr make_identity();
  static ASTNodePtr make_field(const std::string &field_name);
  static ASTNodePtr make_pipe(const ASTNodePtr &left, const ASTNodePtr &right);
};

class Parser {
public:
  explicit Parser(const std::vector<Token> &tokens);

  ASTNodePtr parse();
  std::string error() const { return error_msg; }

private:
  std::vector<Token> tokens;
  size_t pos;
  std::string error_msg;

  Token current() const;
  Token peek(size_t offset = 1) const;
  void advance();
  bool expect(TokenType type);

  // Parsing methods (precedence order)
  ASTNodePtr parse_pipe();
  ASTNodePtr parse_comma();
  ASTNodePtr parse_alternative();
  ASTNodePtr parse_comparison();
  ASTNodePtr parse_additive();
  ASTNodePtr parse_multiplicative();
  ASTNodePtr parse_postfix();
  ASTNodePtr parse_primary();

  // Helper methods
  ASTNodePtr parse_array();
  ASTNodePtr parse_object();
  ASTNodePtr parse_function_call(const std::string &name);
};

} // namespace jq

#endif // JQ_PARSER_HPP
