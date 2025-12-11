#include "jq_parser.hpp"
#include <stdexcept>

namespace jq {

// ================= ASTNode Static Constructors =================

ASTNodePtr ASTNode::make_literal(const JvValuePtr &val) {
  auto node = std::make_shared<ASTNode>(NodeType::LITERAL);
  node->literal = val;
  return node;
}

ASTNodePtr ASTNode::make_identity() {
  return std::make_shared<ASTNode>(NodeType::IDENTITY);
}

ASTNodePtr ASTNode::make_field(const std::string &field_name) {
  auto node = std::make_shared<ASTNode>(NodeType::FIELD);
  node->name = field_name;
  return node;
}

ASTNodePtr ASTNode::make_pipe(const ASTNodePtr &left, const ASTNodePtr &right) {
  auto node = std::make_shared<ASTNode>(NodeType::PIPE);
  node->children.push_back(left);
  node->children.push_back(right);
  return node;
}

// ================= Parser Implementation =================

Parser::Parser(const std::vector<Token> &tokens) : tokens(tokens), pos(0) {}

Token Parser::current() const {
  if (pos >= tokens.size())
    return Token(TokenType::EOF_TOKEN);
  return tokens[pos];
}

Token Parser::peek(size_t offset) const {
  if (pos + offset >= tokens.size())
    return Token(TokenType::EOF_TOKEN);
  return tokens[pos + offset];
}

void Parser::advance() {
  if (pos < tokens.size())
    ++pos;
}

bool Parser::expect(TokenType type) {
  if (current().type != type) {
    error_msg = "Expected token type at line " + std::to_string(current().line);
    return false;
  }
  advance();
  return true;
}

ASTNodePtr Parser::parse() {
  try {
    auto result = parse_pipe();
    if (current().type != TokenType::EOF_TOKEN) {
      error_msg = "Unexpected token after expression";
      return nullptr;
    }
    return result;
  } catch (const std::exception &e) {
    error_msg = e.what();
    return nullptr;
  }
}

// Parse pipe operations: expr | expr
ASTNodePtr Parser::parse_pipe() {
  auto left = parse_comma();

  while (current().type == TokenType::PIPE) {
    advance();
    auto right = parse_comma();
    left = ASTNode::make_pipe(left, right);
  }

  return left;
}

// Parse comma operations: expr, expr
ASTNodePtr Parser::parse_comma() {
  auto left = parse_alternative();

  if (current().type != TokenType::COMMA)
    return left;

  auto node = std::make_shared<ASTNode>(NodeType::COMMA);
  node->children.push_back(left);

  while (current().type == TokenType::COMMA) {
    advance();
    node->children.push_back(parse_alternative());
  }

  return node;
}

// Parse alternative operator: expr // expr
ASTNodePtr Parser::parse_alternative() {
  auto left = parse_comparison();

  while (current().type == TokenType::DOUBLE_SLASH) {
    advance();
    auto right = parse_comparison();
    auto node = std::make_shared<ASTNode>(NodeType::ALTERNATIVE);
    node->children.push_back(left);
    node->children.push_back(right);
    left = node;
  }

  return left;
}

// Parse comparison operators: ==, !=, <, >, <=, >=
ASTNodePtr Parser::parse_comparison() {
  auto left = parse_additive();

  while (current().type == TokenType::EQ || current().type == TokenType::NE ||
         current().type == TokenType::LT || current().type == TokenType::LE ||
         current().type == TokenType::GT || current().type == TokenType::GE) {
    auto op_token = current();
    advance();
    auto right = parse_additive();

    auto node = std::make_shared<ASTNode>(NodeType::BINARY_OP);
    node->op = op_token.value;
    node->children.push_back(left);
    node->children.push_back(right);
    left = node;
  }

  return left;
}

// Parse addition and subtraction: +, -
ASTNodePtr Parser::parse_additive() {
  auto left = parse_multiplicative();

  while (current().type == TokenType::PLUS ||
         current().type == TokenType::MINUS) {
    auto op_token = current();
    advance();
    auto right = parse_multiplicative();

    auto node = std::make_shared<ASTNode>(NodeType::BINARY_OP);
    node->op = op_token.value;
    node->children.push_back(left);
    node->children.push_back(right);
    left = node;
  }

  return left;
}

// Parse multiplication, division, modulo: *, /, %
ASTNodePtr Parser::parse_multiplicative() {
  auto left = parse_postfix();

  while (current().type == TokenType::STAR ||
         current().type == TokenType::SLASH ||
         current().type == TokenType::PERCENT) {
    auto op_token = current();
    advance();
    auto right = parse_postfix();

    auto node = std::make_shared<ASTNode>(NodeType::BINARY_OP);
    node->op = op_token.value;
    node->children.push_back(left);
    node->children.push_back(right);
    left = node;
  }

  return left;
}

// Parse postfix operations: field access, indexing, etc.
ASTNodePtr Parser::parse_postfix() {
  auto base = parse_primary();

  while (true) {
    if (current().type == TokenType::DOT) {
      advance();

      // .identifier
      if (current().type == TokenType::IDENTIFIER) {
        auto field = ASTNode::make_field(current().value);
        advance();

        auto pipe = ASTNode::make_pipe(base, field);
        base = pipe;
      }
      // .[]
      else if (current().type == TokenType::LBRACKET) {
        advance();
        if (current().type == TokenType::RBRACKET) {
          advance();
          auto iter = std::make_shared<ASTNode>(NodeType::ITERATOR);
          base = ASTNode::make_pipe(base, iter);
        } else {
          // .[index] or .[start:end]
          auto index_expr = parse_pipe();

          if (current().type == TokenType::COLON) {
            advance();
            auto end_expr = parse_pipe();
            auto slice = std::make_shared<ASTNode>(NodeType::SLICE);
            slice->children.push_back(index_expr);
            slice->children.push_back(end_expr);
            expect(TokenType::RBRACKET);
            base = ASTNode::make_pipe(base, slice);
          } else {
            expect(TokenType::RBRACKET);
            auto index = std::make_shared<ASTNode>(NodeType::INDEX);
            index->children.push_back(index_expr);
            base = ASTNode::make_pipe(base, index);
          }
        }
      }
      // Just . (identity on result of base)
      else {
        auto id = ASTNode::make_identity();
        base = ASTNode::make_pipe(base, id);
      }
    }
    // Direct [index] without dot
    else if (current().type == TokenType::LBRACKET) {
      advance();
      if (current().type == TokenType::RBRACKET) {
        advance();
        auto iter = std::make_shared<ASTNode>(NodeType::ITERATOR);
        base = ASTNode::make_pipe(base, iter);
      } else {
        auto index_expr = parse_pipe();
        expect(TokenType::RBRACKET);
        auto index = std::make_shared<ASTNode>(NodeType::INDEX);
        index->children.push_back(index_expr);
        base = ASTNode::make_pipe(base, index);
      }
    } else {
      break;
    }
  }

  return base;
}

// Parse primary expressions
ASTNodePtr Parser::parse_primary() {
  auto tok = current();

  // Literals
  if (tok.type == TokenType::NUMBER) {
    advance();
    double num = std::stod(tok.value);
    return ASTNode::make_literal(JvValue::number(num));
  }

  if (tok.type == TokenType::STRING) {
    advance();
    return ASTNode::make_literal(JvValue::string(tok.value));
  }

  if (tok.type == TokenType::TRUE) {
    advance();
    return ASTNode::make_literal(JvValue::boolean(true));
  }

  if (tok.type == TokenType::FALSE) {
    advance();
    return ASTNode::make_literal(JvValue::boolean(false));
  }

  if (tok.type == TokenType::NULL_VALUE) {
    advance();
    return ASTNode::make_literal(JvValue::null());
  }

  // Identity
  if (tok.type == TokenType::DOT) {
    advance();

    // .identifier
    if (current().type == TokenType::IDENTIFIER) {
      auto field = ASTNode::make_field(current().value);
      advance();
      return field;
    }
    // .[]
    else if (current().type == TokenType::LBRACKET) {
      advance();
      if (current().type == TokenType::RBRACKET) {
        advance();
        return std::make_shared<ASTNode>(NodeType::ITERATOR);
      } else {
        auto index_expr = parse_pipe();
        expect(TokenType::RBRACKET);
        auto index = std::make_shared<ASTNode>(NodeType::INDEX);
        index->children.push_back(index_expr);
        return index;
      }
    }
    // Just .
    else {
      return ASTNode::make_identity();
    }
  }

  // Recursive descent
  if (tok.type == TokenType::RECURSIVE) {
    advance();
    return std::make_shared<ASTNode>(NodeType::RECURSIVE);
  }

  // Parenthesized expression
  if (tok.type == TokenType::LPAREN) {
    advance();
    auto expr = parse_pipe();
    expect(TokenType::RPAREN);
    return expr;
  }

  // Array constructor
  if (tok.type == TokenType::LBRACKET) {
    return parse_array();
  }

  // Object constructor
  if (tok.type == TokenType::LBRACE) {
    return parse_object();
  }

  // Function call or identifier
  if (tok.type == TokenType::IDENTIFIER) {
    std::string name = tok.value;
    advance();

    if (current().type == TokenType::LPAREN) {
      return parse_function_call(name);
    }

    // Bare identifier is a function with no args
    auto node = std::make_shared<ASTNode>(NodeType::FUNCTION_CALL);
    node->name = name;
    return node;
  }

  // Unary minus
  if (tok.type == TokenType::MINUS) {
    advance();
    auto operand = parse_postfix();
    auto node = std::make_shared<ASTNode>(NodeType::UNARY_OP);
    node->op = "-";
    node->children.push_back(operand);
    return node;
  }

  // NOT
  if (tok.type == TokenType::NOT) {
    advance();
    auto operand = parse_postfix();
    auto node = std::make_shared<ASTNode>(NodeType::UNARY_OP);
    node->op = "not";
    node->children.push_back(operand);
    return node;
  }

  error_msg = "Unexpected token in primary: " + tok.value;
  throw std::runtime_error(error_msg);
}

ASTNodePtr Parser::parse_array() {
  expect(TokenType::LBRACKET);

  auto node = std::make_shared<ASTNode>(NodeType::ARRAY);

  if (current().type != TokenType::RBRACKET) {
    node->children.push_back(parse_pipe());
  }

  expect(TokenType::RBRACKET);
  return node;
}

ASTNodePtr Parser::parse_object() {
  expect(TokenType::LBRACE);

  auto node = std::make_shared<ASTNode>(NodeType::OBJECT);

  while (current().type != TokenType::RBRACE &&
         current().type != TokenType::EOF_TOKEN) {
    // Parse key
    ASTNodePtr key;
    if (current().type == TokenType::STRING) {
      key = ASTNode::make_literal(JvValue::string(current().value));
      advance();
    } else if (current().type == TokenType::IDENTIFIER) {
      key = ASTNode::make_literal(JvValue::string(current().value));
      advance();
    } else if (current().type == TokenType::LPAREN) {
      advance();
      key = parse_pipe();
      expect(TokenType::RPAREN);
    } else {
      break;
    }

    expect(TokenType::COLON);

    // Parse value
    auto value = parse_pipe();

    node->children.push_back(key);
    node->children.push_back(value);

    if (current().type == TokenType::COMMA) {
      advance();
    } else {
      break;
    }
  }

  expect(TokenType::RBRACE);
  return node;
}

ASTNodePtr Parser::parse_function_call(const std::string &name) {
  expect(TokenType::LPAREN);

  auto node = std::make_shared<ASTNode>(NodeType::FUNCTION_CALL);
  node->name = name;

  if (current().type != TokenType::RPAREN) {
    node->children.push_back(parse_pipe());

    while (current().type == TokenType::SEMICOLON) {
      advance();
      node->children.push_back(parse_pipe());
    }
  }

  expect(TokenType::RPAREN);
  return node;
}

} // namespace jq
