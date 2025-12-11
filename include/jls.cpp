#include "jls.hpp"
#include "jls_library.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>

namespace jls {

static std::string str_tolower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return out;
}

// ================= Lexer Implementation =================

Lexer::Lexer(const std::string &source)
    : source(source), position(0), line(1), column(1) {}

char Lexer::current_char() {
  if (position >= source.size())
    return '\0';
  return source[position];
}

char Lexer::peek_char(size_t offset) {
  if (position + offset >= source.size())
    return '\0';
  return source[position + offset];
}

void Lexer::advance() {
  if (position < source.size()) {
    if (source[position] == '\n') {
      ++line;
      column = 1;
    } else {
      ++column;
    }
    ++position;
  }
}

void Lexer::skip_whitespace() {
  while (current_char() == ' ' || current_char() == '\t' ||
         current_char() == '\r') {
    advance();
  }
}

void Lexer::skip_comment() {
  if (current_char() == '\'' ||
      (current_char() == 'R' && peek_char() == 'E' && peek_char(2) == 'M')) {
    while (current_char() != '\n' && current_char() != '\0') {
      advance();
    }
  }
}

Token Lexer::read_number() {
  size_t start_line = line;
  size_t start_col = column;
  std::string number;

  bool is_float = false;
  while (std::isdigit(static_cast<unsigned char>(current_char())) ||
         current_char() == '.') {
    if (current_char() == '.') {
      if (is_float)
        break;
      is_float = true;
    }
    number += current_char();
    advance();
  }

  if (is_float) {
    return Token(TokenType::FLOAT, number, start_line, start_col);
  } else {
    return Token(TokenType::INTEGER, number, start_line, start_col);
  }
}

Token Lexer::read_string() {
  size_t start_line = line;
  size_t start_col = column;
  advance(); // Skip opening quote
  std::string string_val;

  while (current_char() != '"' && current_char() != '\0') {
    if (current_char() == '\\') {
      advance();
      switch (current_char()) {
      case 'n':
        string_val += '\n';
        break;
      case 't':
        string_val += '\t';
        break;
      case 'r':
        string_val += '\r';
        break;
      case '\\':
        string_val += '\\';
        break;
      case '"':
        string_val += '"';
        break;
      default:
        string_val += current_char();
      }
      advance();
    } else {
      string_val += current_char();
      advance();
    }
  }

  if (current_char() == '"')
    advance(); // Skip closing quote

  return Token(TokenType::STRING, string_val, start_line, start_col);
}

Token Lexer::read_identifier() {
  size_t start_line = line;
  size_t start_col = column;
  std::string identifier;

  while (std::isalnum(static_cast<unsigned char>(current_char())) ||
         current_char() == '_') {
    identifier += current_char();
    advance();
  }

  // Convert to uppercase for case-insensitive keywords
  std::string upper = identifier;
  std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

  // Check for keywords
  if (upper == "PRINT")
    return Token(TokenType::PRINT, identifier, start_line, start_col);
  if (upper == "LET")
    return Token(TokenType::LET, identifier, start_line, start_col);
  if (upper == "IF")
    return Token(TokenType::IF, identifier, start_line, start_col);
  if (upper == "THEN")
    return Token(TokenType::THEN, identifier, start_line, start_col);
  if (upper == "ELSE")
    return Token(TokenType::ELSE, identifier, start_line, start_col);
  if (upper == "END")
    return Token(TokenType::END, identifier, start_line, start_col);
  if (upper == "FOR")
    return Token(TokenType::FOR, identifier, start_line, start_col);
  if (upper == "TO")
    return Token(TokenType::TO, identifier, start_line, start_col);
  if (upper == "STEP")
    return Token(TokenType::STEP, identifier, start_line, start_col);
  if (upper == "NEXT")
    return Token(TokenType::NEXT, identifier, start_line, start_col);
  if (upper == "WHILE")
    return Token(TokenType::WHILE, identifier, start_line, start_col);
  if (upper == "DO")
    return Token(TokenType::DO, identifier, start_line, start_col);
  if (upper == "FUNCTION")
    return Token(TokenType::FUNCTION, identifier, start_line, start_col);
  if (upper == "CALL")
    return Token(TokenType::CALL, identifier, start_line, start_col);
  if (upper == "RETURN")
    return Token(TokenType::RETURN, identifier, start_line, start_col);
  if (upper == "TRUE")
    return Token(TokenType::TRUE, identifier, start_line, start_col);
  if (upper == "FALSE")
    return Token(TokenType::FALSE, identifier, start_line, start_col);
  if (upper == "NIL")
    return Token(TokenType::NIL, identifier, start_line, start_col);
  if (upper == "AND")
    return Token(TokenType::AND, identifier, start_line, start_col);
  if (upper == "OR")
    return Token(TokenType::OR, identifier, start_line, start_col);
  if (upper == "NOT")
    return Token(TokenType::NOT, identifier, start_line, start_col);

  return Token(TokenType::IDENTIFIER, identifier, start_line, start_col);
}

Token Lexer::next_token() {
  skip_whitespace();

  // Skip comments
  while (current_char() == '\'' ||
         (current_char() == 'R' && peek_char() == 'E' && peek_char(2) == 'M')) {
    skip_comment();
    skip_whitespace();
  }

  size_t token_line = line;
  size_t token_col = column;
  char ch = current_char();

  if (ch == '\0')
    return Token(TokenType::EOF_TOKEN, "", token_line, token_col);

  if (ch == '\n') {
    advance();
    return Token(TokenType::NEWLINE, "\\n", token_line, token_col);
  }

  // Single character tokens
  if (ch == '(') {
    advance();
    return Token(TokenType::LPAREN, "(", token_line, token_col);
  }
  if (ch == ')') {
    advance();
    return Token(TokenType::RPAREN, ")", token_line, token_col);
  }
  if (ch == '[') {
    advance();
    return Token(TokenType::LBRACKET, "[", token_line, token_col);
  }
  if (ch == ']') {
    advance();
    return Token(TokenType::RBRACKET, "]", token_line, token_col);
  }
  if (ch == ',') {
    advance();
    return Token(TokenType::COMMA, ",", token_line, token_col);
  }
  if (ch == '+') {
    advance();
    return Token(TokenType::PLUS, "+", token_line, token_col);
  }
  if (ch == '-') {
    advance();
    return Token(TokenType::MINUS, "-", token_line, token_col);
  }
  if (ch == '*') {
    advance();
    return Token(TokenType::STAR, "*", token_line, token_col);
  }
  if (ch == '/') {
    advance();
    return Token(TokenType::SLASH, "/", token_line, token_col);
  }
  if (ch == '%') {
    advance();
    return Token(TokenType::PERCENT, "%", token_line, token_col);
  }
  if (ch == '^') {
    advance();
    return Token(TokenType::CARET, "^", token_line, token_col);
  }

  // Two character operators
  if (ch == '<') {
    advance();
    if (current_char() == '=') {
      advance();
      return Token(TokenType::LTE, "<=", token_line, token_col);
    } else if (current_char() == '>') {
      advance();
      return Token(TokenType::NEQ, "<>", token_line, token_col);
    }
    return Token(TokenType::LT, "<", token_line, token_col);
  }

  if (ch == '>') {
    advance();
    if (current_char() == '=') {
      advance();
      return Token(TokenType::GTE, ">=", token_line, token_col);
    }
    return Token(TokenType::GT, ">", token_line, token_col);
  }

  if (ch == '=') {
    advance();
    if (current_char() == '=') {
      advance();
      return Token(TokenType::EQ, "==", token_line, token_col);
    }
    return Token(TokenType::EQUALS, "=", token_line, token_col);
  }

  // Numbers
  if (std::isdigit(static_cast<unsigned char>(ch))) {
    return read_number();
  }

  // Strings
  if (ch == '"') {
    return read_string();
  }

  // Identifiers
  if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
    return read_identifier();
  }

  advance();
  return Token(TokenType::ERROR, std::string(1, ch), token_line, token_col);
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  Token tok = next_token();
  while (tok.type != TokenType::EOF_TOKEN) {
    // Skip newlines for now (can handle them later for multi-line statements)
    if (tok.type != TokenType::NEWLINE) {
      tokens.push_back(tok);
    }
    tok = next_token();
  }
  tokens.push_back(tok); // Add EOF token
  return tokens;
}

// ================= Parser Implementation =================

Parser::Parser(const std::vector<Token> &tokens)
    : tokens(tokens), position(0) {}

Token Parser::current_token() {
  if (position < tokens.size())
    return tokens[position];
  return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

Token Parser::peek_token(size_t offset) {
  if (position + offset < tokens.size())
    return tokens[position + offset];
  return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

void Parser::advance() {
  if (position < tokens.size())
    ++position;
}

bool Parser::expect(TokenType type) {
  if (current_token().type != type) {
    error_msg = "Expected different token type";
    return false;
  }
  advance();
  return true;
}

ASTNodePtr Parser::parse() {
  if (current_token().type == TokenType::EOF_TOKEN) {
    auto nil_node = std::make_shared<ASTNode>();
    nil_node->type = NodeType::LITERAL;
    nil_node->literal_value = std::make_shared<Value>();
    return nil_node;
  }
  return parse_statement();
}

ASTNodePtr Parser::parse_statement() {
  Token tok = current_token();

  switch (tok.type) {
  case TokenType::PRINT:
    return parse_print();
  case TokenType::LET:
    return parse_let();
  case TokenType::IF:
    return parse_if();
  case TokenType::FOR:
    return parse_for();
  case TokenType::WHILE:
    return parse_while();
  case TokenType::FUNCTION:
    return parse_function();
  case TokenType::IDENTIFIER:
    // Could be assignment or function call
    if (peek_token().type == TokenType::EQUALS) {
      return parse_assignment();
    } else if (peek_token().type == TokenType::LPAREN) {
      return parse_expression(); // Function call
    }
    return parse_expression();
  default:
    return parse_expression();
  }
}

ASTNodePtr Parser::parse_print() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::PRINT;
  advance(); // Skip PRINT

  // Parse expression to print
  node->children.push_back(parse_expression());

  return node;
}

ASTNodePtr Parser::parse_let() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::LET;
  advance(); // Skip LET

  if (current_token().type != TokenType::IDENTIFIER) {
    error_msg = "Expected identifier after LET";
    return node;
  }

  node->identifier_name = current_token().value;
  advance();

  if (current_token().type != TokenType::EQUALS) {
    error_msg = "Expected = after identifier";
    return node;
  }
  advance();

  node->children.push_back(parse_expression());

  return node;
}

ASTNodePtr Parser::parse_assignment() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::ASSIGNMENT;

  node->identifier_name = current_token().value;
  advance();

  expect(TokenType::EQUALS);

  node->children.push_back(parse_expression());

  return node;
}

ASTNodePtr Parser::parse_if() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::IF_STMT;
  advance(); // Skip IF

  node->condition = parse_expression();

  if (current_token().type == TokenType::THEN) {
    advance();
  }

  // Parse then branch (can be a statement or expression)
  // Check if it's a statement keyword
  if (current_token().type == TokenType::PRINT ||
      current_token().type == TokenType::LET ||
      current_token().type == TokenType::IDENTIFIER) {
    node->then_branch = parse_statement();
  } else {
    node->then_branch = parse_expression();
  }

  // Parse optional else branch
  if (current_token().type == TokenType::ELSE) {
    advance();
    if (current_token().type == TokenType::PRINT ||
        current_token().type == TokenType::LET ||
        current_token().type == TokenType::IDENTIFIER) {
      node->else_branch = parse_statement();
    } else {
      node->else_branch = parse_expression();
    }
  }

  // Skip optional END keyword
  if (current_token().type == TokenType::END) {
    advance();
  }

  return node;
}

ASTNodePtr Parser::parse_for() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::FOR_LOOP;
  advance(); // Skip FOR

  // Parse loop variable
  if (current_token().type != TokenType::IDENTIFIER) {
    error_msg = "Expected identifier after FOR";
    return node;
  }

  node->identifier_name = current_token().value;
  advance();

  expect(TokenType::EQUALS);

  // Start value
  node->children.push_back(parse_expression());

  expect(TokenType::TO);

  // End value
  node->children.push_back(parse_expression());

  // Optional STEP
  if (current_token().type == TokenType::STEP) {
    advance();
    node->children.push_back(parse_expression());
  }

  // Parse body (simplified for now)
  // In full implementation, would parse until NEXT

  return node;
}

ASTNodePtr Parser::parse_while() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::WHILE_LOOP;
  advance(); // Skip WHILE

  node->condition = parse_expression();

  if (current_token().type == TokenType::DO) {
    advance();
  }

  // Parse body (simplified)

  return node;
}

ASTNodePtr Parser::parse_function() {
  auto node = std::make_shared<ASTNode>();
  node->type = NodeType::FUNCTION_DEF;
  advance(); // Skip FUNCTION

  if (current_token().type != TokenType::IDENTIFIER) {
    error_msg = "Expected function name";
    return node;
  }

  node->identifier_name = current_token().value;
  advance();

  // Parse parameters
  if (current_token().type == TokenType::LPAREN) {
    advance();
    while (current_token().type != TokenType::RPAREN &&
           current_token().type != TokenType::EOF_TOKEN) {
      if (current_token().type == TokenType::IDENTIFIER) {
        // Store parameter names (simplified)
        advance();
      }
      if (current_token().type == TokenType::COMMA) {
        advance();
      }
    }
    expect(TokenType::RPAREN);
  }

  return node;
}

ASTNodePtr Parser::parse_expression() { return parse_comparison(); }

ASTNodePtr Parser::parse_comparison() {
  auto left = parse_term();

  while (current_token().type == TokenType::LT ||
         current_token().type == TokenType::GT ||
         current_token().type == TokenType::LTE ||
         current_token().type == TokenType::GTE ||
         current_token().type == TokenType::EQ ||
         current_token().type == TokenType::NEQ ||
         current_token().type == TokenType::AND ||
         current_token().type == TokenType::OR) {
    auto node = std::make_shared<ASTNode>();
    node->type = NodeType::BINARY_OP;
    node->op = current_token().value;
    advance();

    node->children.push_back(left);
    node->children.push_back(parse_term());
    left = node;
  }

  return left;
}

ASTNodePtr Parser::parse_term() {
  auto left = parse_factor();

  while (current_token().type == TokenType::PLUS ||
         current_token().type == TokenType::MINUS) {
    auto node = std::make_shared<ASTNode>();
    node->type = NodeType::BINARY_OP;
    node->op = current_token().value;
    advance();

    node->children.push_back(left);
    node->children.push_back(parse_factor());
    left = node;
  }

  return left;
}

ASTNodePtr Parser::parse_factor() {
  auto left = parse_primary();

  while (current_token().type == TokenType::STAR ||
         current_token().type == TokenType::SLASH ||
         current_token().type == TokenType::PERCENT ||
         current_token().type == TokenType::CARET) {
    auto node = std::make_shared<ASTNode>();
    node->type = NodeType::BINARY_OP;
    node->op = current_token().value;
    advance();

    node->children.push_back(left);
    node->children.push_back(parse_primary());
    left = node;
  }

  return left;
}

ASTNodePtr Parser::parse_primary() {
  Token tok = current_token();
  auto node = std::make_shared<ASTNode>();

  switch (tok.type) {
  case TokenType::INTEGER: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>(std::stoll(tok.value));
    advance();
    break;
  }
  case TokenType::FLOAT: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>(std::stod(tok.value));
    advance();
    break;
  }
  case TokenType::STRING: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>(tok.value);
    advance();
    break;
  }
  case TokenType::TRUE: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>(true);
    advance();
    break;
  }
  case TokenType::FALSE: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>(false);
    advance();
    break;
  }
  case TokenType::NIL: {
    node->type = NodeType::LITERAL;
    node->literal_value = std::make_shared<Value>();
    advance();
    break;
  }
  case TokenType::IDENTIFIER: {
    std::string name = tok.value;
    advance();

    // Check for function call
    if (LibraryLoader::is_library_name(name) &&
      current_token().type == TokenType::SLASH &&
        peek_token().type == TokenType::IDENTIFIER &&
        peek_token(2).type == TokenType::LPAREN) {
      // Namespaced call: lib/func()
      std::string func_name = name + "/" + peek_token().value;
      advance(); // skip '/'
      advance(); // skip second identifier

      node->type = NodeType::FUNCTION_CALL;
      node->identifier_name = func_name;
      advance(); // Skip (

      // Parse arguments
      while (current_token().type != TokenType::RPAREN &&
             current_token().type != TokenType::EOF_TOKEN) {
        node->children.push_back(parse_expression());
        if (current_token().type == TokenType::COMMA) {
          advance();
        }
      }
      expect(TokenType::RPAREN);
    } else if (current_token().type == TokenType::LPAREN) {
      node->type = NodeType::FUNCTION_CALL;
      node->identifier_name = name;
      advance(); // Skip (

      // Parse arguments
      while (current_token().type != TokenType::RPAREN &&
             current_token().type != TokenType::EOF_TOKEN) {
        node->children.push_back(parse_expression());
        if (current_token().type == TokenType::COMMA) {
          advance();
        }
      }
      expect(TokenType::RPAREN);
    } else {
      // Variable reference
      node->type = NodeType::IDENTIFIER;
      node->identifier_name = name;
    }
    break;
  }
  case TokenType::LPAREN: {
    advance(); // Skip (
    node = parse_expression();
    expect(TokenType::RPAREN);
    break;
  }
  case TokenType::NOT: {
    node->type = NodeType::UNARY_OP;
    node->op = "NOT";
    advance();
    node->children.push_back(parse_primary());
    break;
  }
  case TokenType::MINUS: {
    node->type = NodeType::UNARY_OP;
    node->op = "-";
    advance();
    node->children.push_back(parse_primary());
    break;
  }
  default: {
    error_msg = "Unexpected token in parse_primary";
    break;
  }
  }

  return node;
}

// ================= Environment Implementation =================

ValuePtr Environment::get(const std::string &name) {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }
  if (parent) {
    return parent->get(name);
  }
  return nullptr;
}

void Environment::set(const std::string &name, const ValuePtr &value) {
  variables[name] = value;
}

bool Environment::exists(const std::string &name) const {
  if (variables.find(name) != variables.end()) {
    return true;
  }
  if (parent) {
    return parent->exists(name);
  }
  return false;
}

// ================= Evaluator Implementation =================

Evaluator::Evaluator() {
  global_env = std::make_shared<Environment>();
  BSC::register_functions(global_env);
}

ValuePtr Evaluator::eval(const ASTNodePtr &node, EnvironmentPtr env) {
  error_msg.clear(); // Clear previous error message
  if (!env) {
    env = global_env;
  }
  return eval_node(node, env);
}

ValuePtr Evaluator::eval_node(const ASTNodePtr &node, EnvironmentPtr env) {
  if (!node) {
    return std::make_shared<Value>();
  }

  switch (node->type) {
  case NodeType::LITERAL: {
    return node->literal_value;
  }
  case NodeType::IDENTIFIER: {
    auto val = env->get(node->identifier_name);
    if (!val) {
      error_msg = "Undefined variable: " + node->identifier_name;
      return std::make_shared<Value>();
    }
    return val;
  }
  case NodeType::BINARY_OP: {
    if (node->children.size() < 2) {
      error_msg = "Binary operation requires two operands";
      return std::make_shared<Value>();
    }

    auto left = eval_node(node->children[0], env);
    auto right = eval_node(node->children[1], env);

    // Arithmetic operations
    if (node->op == "+") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        return std::make_shared<Value>(left->i + right->i);
      } else {
        double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
        double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
        return std::make_shared<Value>(l + r);
      }
    }
    if (node->op == "-") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        return std::make_shared<Value>(left->i - right->i);
      } else {
        double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
        double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
        return std::make_shared<Value>(l - r);
      }
    }
    if (node->op == "*") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        return std::make_shared<Value>(left->i * right->i);
      } else {
        double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
        double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
        return std::make_shared<Value>(l * r);
      }
    }
    if (node->op == "/") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      if (r == 0.0) {
        error_msg = "Division by zero";
        return std::make_shared<Value>(0.0);
      }
      return std::make_shared<Value>(l / r);
    }
    if (node->op == "%") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        if (right->i == 0) {
          error_msg = "Modulo by zero";
          return std::make_shared<Value>(0LL);
        }
        return std::make_shared<Value>(left->i % right->i);
      }
    }
    if (node->op == "^") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      return std::make_shared<Value>(std::pow(l, r));
    }

    // Comparison operations
    if (node->op == "<") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      return std::make_shared<Value>(l < r);
    }
    if (node->op == ">") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      return std::make_shared<Value>(l > r);
    }
    if (node->op == "<=") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      return std::make_shared<Value>(l <= r);
    }
    if (node->op == ">=") {
      double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
      double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
      return std::make_shared<Value>(l >= r);
    }
    if (node->op == "==" || node->op == "=") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        return std::make_shared<Value>(left->i == right->i);
      } else if (left->type == ValueType::FLOAT ||
                 right->type == ValueType::FLOAT) {
        double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
        double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
        return std::make_shared<Value>(l == r);
      }
    }
    if (node->op == "<>") {
      if (left->type == ValueType::INTEGER &&
          right->type == ValueType::INTEGER) {
        return std::make_shared<Value>(left->i != right->i);
      } else if (left->type == ValueType::FLOAT ||
                 right->type == ValueType::FLOAT) {
        double l = (left->type == ValueType::INTEGER) ? left->i : left->f;
        double r = (right->type == ValueType::INTEGER) ? right->i : right->f;
        return std::make_shared<Value>(l != r);
      }
    }

    // Logical operations
    if (node->op == "AND" || node->op == "and") {
      bool l = (left->type == ValueType::BOOLEAN)
                   ? left->b
                   : (left->type != ValueType::NIL);
      bool r = (right->type == ValueType::BOOLEAN)
                   ? right->b
                   : (right->type != ValueType::NIL);
      return std::make_shared<Value>(l && r);
    }
    if (node->op == "OR" || node->op == "or") {
      bool l = (left->type == ValueType::BOOLEAN)
                   ? left->b
                   : (left->type != ValueType::NIL);
      bool r = (right->type == ValueType::BOOLEAN)
                   ? right->b
                   : (right->type != ValueType::NIL);
      return std::make_shared<Value>(l || r);
    }

    return std::make_shared<Value>();
  }
  case NodeType::UNARY_OP: {
    if (node->children.empty()) {
      error_msg = "Unary operation requires one operand";
      return std::make_shared<Value>();
    }

    auto operand = eval_node(node->children[0], env);

    if (node->op == "-") {
      if (operand->type == ValueType::INTEGER) {
        return std::make_shared<Value>(-operand->i);
      } else if (operand->type == ValueType::FLOAT) {
        return std::make_shared<Value>(-operand->f);
      }
    }
    if (node->op == "NOT" || node->op == "not") {
      bool val = (operand->type == ValueType::BOOLEAN)
                     ? operand->b
                     : (operand->type != ValueType::NIL);
      return std::make_shared<Value>(!val);
    }

    return std::make_shared<Value>();
  }
  case NodeType::PRINT: {
    if (node->children.empty()) {
      std::cout << std::endl;
      return std::make_shared<Value>();
    }

    auto val = eval_node(node->children[0], env);
    if (val->type == ValueType::STRING) {
      std::cout << val->s << std::endl;
    } else if (val->type == ValueType::INTEGER) {
      std::cout << val->i << std::endl;
    } else if (val->type == ValueType::FLOAT) {
      std::cout << val->f << std::endl;
    } else if (val->type == ValueType::BOOLEAN) {
      std::cout << (val->b ? "true" : "false") << std::endl;
    } else if (val->type == ValueType::NIL) {
      std::cout << "nil" << std::endl;
    }

    return val;
  }
  case NodeType::LET: {
    if (node->children.empty()) {
      error_msg = "LET requires a value";
      return std::make_shared<Value>();
    }

    auto val = eval_node(node->children[0], env);
    env->set(node->identifier_name, val);
    return val;
  }
  case NodeType::ASSIGNMENT: {
    if (node->children.empty()) {
      error_msg = "Assignment requires a value";
      return std::make_shared<Value>();
    }

    auto val = eval_node(node->children[0], env);
    env->set(node->identifier_name, val);
    return val;
  }
  case NodeType::IF_STMT: {
    auto cond = eval_node(node->condition, env);
    bool condition = (cond->type == ValueType::BOOLEAN)
                         ? cond->b
                         : (cond->type != ValueType::NIL);

    if (condition && node->then_branch) {
      return eval_node(node->then_branch, env);
    } else if (!condition && node->else_branch) {
      return eval_node(node->else_branch, env);
    }

    return std::make_shared<Value>();
  }
  case NodeType::FUNCTION_CALL: {
    // Get function
    auto func_val = env->get(node->identifier_name);

    // Support namespaced calls like math/sin()
    if (!func_val) {
      auto sep = node->identifier_name.find('/');
      if (sep != std::string::npos) {
        std::string lib_name = str_tolower(node->identifier_name.substr(0, sep));
        std::string inner_name =
            str_tolower(node->identifier_name.substr(sep + 1));
        auto lib_val = env->get(lib_name);
        if (lib_val && lib_val->type == ValueType::MAP) {
          auto it = lib_val->map.find(inner_name);
          if (it != lib_val->map.end()) {
            func_val = it->second;
          }
        }
      }
    }

    if (!func_val) {
      error_msg = "Undefined function: " + node->identifier_name;
      return std::make_shared<Value>();
    }

    // Evaluate arguments
    std::vector<ValuePtr> args;
    for (const auto &arg_node : node->children) {
      args.push_back(eval_node(arg_node, env));
    }

    // Call native function
    if (func_val->native_func) {
      return func_val->native_func(args);
    }

    error_msg = "Not a callable function";
    return std::make_shared<Value>();
  }
  default:
    return std::make_shared<Value>();
  }
}

ValuePtr Evaluator::call_function(const ValuePtr &func,
                                  const std::vector<ValuePtr> &args,
                                  EnvironmentPtr env) {
  if (func->native_func) {
    return func->native_func(args);
  }

  error_msg = "Invalid function";
  return std::make_shared<Value>();
}

// ================= BSC Implementation =================

void BSC::register_functions(EnvironmentPtr env) {
  // Math functions
  auto abs_val = std::make_shared<Value>();
  abs_val->type = ValueType::FUNCTION;
  abs_val->native_func = fn_abs;
  env->set("ABS", abs_val);

  auto sqrt_val = std::make_shared<Value>();
  sqrt_val->type = ValueType::FUNCTION;
  sqrt_val->native_func = fn_sqrt;
  env->set("SQRT", sqrt_val);

  auto pow_val = std::make_shared<Value>();
  pow_val->type = ValueType::FUNCTION;
  pow_val->native_func = fn_pow;
  env->set("POW", pow_val);

  auto floor_val = std::make_shared<Value>();
  floor_val->type = ValueType::FUNCTION;
  floor_val->native_func = fn_floor;
  env->set("FLOOR", floor_val);

  auto ceil_val = std::make_shared<Value>();
  ceil_val->type = ValueType::FUNCTION;
  ceil_val->native_func = fn_ceil;
  env->set("CEIL", ceil_val);

  auto min_val = std::make_shared<Value>();
  min_val->type = ValueType::FUNCTION;
  min_val->native_func = fn_min;
  env->set("MIN", min_val);

  auto max_val = std::make_shared<Value>();
  max_val->type = ValueType::FUNCTION;
  max_val->native_func = fn_max;
  env->set("MAX", max_val);

  auto random_val = std::make_shared<Value>();
  random_val->type = ValueType::FUNCTION;
  random_val->native_func = fn_random;
  env->set("RANDOM", random_val);
  env->set("RND", random_val); // Alias

  // String functions
  auto len_val = std::make_shared<Value>();
  len_val->type = ValueType::FUNCTION;
  len_val->native_func = fn_length;
  env->set("LEN", len_val);

  auto str_val = std::make_shared<Value>();
  str_val->type = ValueType::FUNCTION;
  str_val->native_func = fn_string;
  env->set("STR", str_val);

  // I/O functions
  auto input_val = std::make_shared<Value>();
  input_val->type = ValueType::FUNCTION;
  input_val->native_func = fn_input;
  env->set("INPUT", input_val);

  auto type_val = std::make_shared<Value>();
  type_val->type = ValueType::FUNCTION;
  type_val->native_func = fn_type;
  env->set("TYPE", type_val);

  // Conversion functions
  auto int_val = std::make_shared<Value>();
  int_val->type = ValueType::FUNCTION;
  int_val->native_func = fn_int;
  env->set("INT", int_val);

  auto float_val = std::make_shared<Value>();
  float_val->type = ValueType::FUNCTION;
  float_val->native_func = fn_float;
  env->set("FLOAT", float_val);
}

ValuePtr BSC::fn_abs(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0LL);

  if (args[0]->type == ValueType::INTEGER) {
    return std::make_shared<Value>(std::abs(args[0]->i));
  } else if (args[0]->type == ValueType::FLOAT) {
    return std::make_shared<Value>(std::abs(args[0]->f));
  }

  return std::make_shared<Value>(0LL);
}

ValuePtr BSC::fn_sqrt(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);

  double val = 0.0;
  if (args[0]->type == ValueType::INTEGER)
    val = args[0]->i;
  else if (args[0]->type == ValueType::FLOAT)
    val = args[0]->f;

  return std::make_shared<Value>(std::sqrt(val));
}

ValuePtr BSC::fn_pow(const std::vector<ValuePtr> &args) {
  if (args.size() < 2)
    return std::make_shared<Value>(1.0);

  double base = 0.0, exp = 0.0;
  if (args[0]->type == ValueType::INTEGER)
    base = args[0]->i;
  else if (args[0]->type == ValueType::FLOAT)
    base = args[0]->f;

  if (args[1]->type == ValueType::INTEGER)
    exp = args[1]->i;
  else if (args[1]->type == ValueType::FLOAT)
    exp = args[1]->f;

  return std::make_shared<Value>(std::pow(base, exp));
}

ValuePtr BSC::fn_floor(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);

  double val = 0.0;
  if (args[0]->type == ValueType::INTEGER)
    return args[0];
  else if (args[0]->type == ValueType::FLOAT)
    val = args[0]->f;

  return std::make_shared<Value>(std::floor(val));
}

ValuePtr BSC::fn_ceil(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);

  double val = 0.0;
  if (args[0]->type == ValueType::INTEGER)
    return args[0];
  else if (args[0]->type == ValueType::FLOAT)
    val = args[0]->f;

  return std::make_shared<Value>(std::ceil(val));
}

ValuePtr BSC::fn_min(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0LL);

  double min_val = 0.0;
  bool first = true;

  for (const auto &arg : args) {
    double val = 0.0;
    if (arg->type == ValueType::INTEGER)
      val = arg->i;
    else if (arg->type == ValueType::FLOAT)
      val = arg->f;

    if (first || val < min_val) {
      min_val = val;
      first = false;
    }
  }

  return std::make_shared<Value>(min_val);
}

ValuePtr BSC::fn_max(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0LL);

  double max_val = 0.0;
  bool first = true;

  for (const auto &arg : args) {
    double val = 0.0;
    if (arg->type == ValueType::INTEGER)
      val = arg->i;
    else if (arg->type == ValueType::FLOAT)
      val = arg->f;

    if (first || val > max_val) {
      max_val = val;
      first = false;
    }
  }

  return std::make_shared<Value>(max_val);
}

ValuePtr BSC::fn_string(const std::vector<ValuePtr> &args) {
  std::string result;
  for (const auto &arg : args) {
    if (arg->type == ValueType::STRING) {
      result += arg->s;
    } else if (arg->type == ValueType::INTEGER) {
      result += std::to_string(arg->i);
    } else if (arg->type == ValueType::FLOAT) {
      result += std::to_string(arg->f);
    }
  }
  return std::make_shared<Value>(result);
}

ValuePtr BSC::fn_length(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0LL);

  if (args[0]->type == ValueType::STRING) {
    return std::make_shared<Value>(static_cast<long long>(args[0]->s.size()));
  } else if (args[0]->type == ValueType::LIST) {
    return std::make_shared<Value>(
        static_cast<long long>(args[0]->list.size()));
  }

  return std::make_shared<Value>(0LL);
}

ValuePtr BSC::fn_input(const std::vector<ValuePtr> &args) {
  // Optional prompt
  if (!args.empty() && args[0]->type == ValueType::STRING) {
    std::cout << args[0]->s;
  }

  std::string result;
  std::getline(std::cin, result);
  return std::make_shared<Value>(result);
}

ValuePtr BSC::fn_random(const std::vector<ValuePtr> &args) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  if (args.empty()) {
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return std::make_shared<Value>(dis(gen));
  }

  long long max_val = 100;
  if (args[0]->type == ValueType::INTEGER) {
    max_val = args[0]->i;
  }

  std::uniform_int_distribution<> dis(0, max_val - 1);
  return std::make_shared<Value>(static_cast<long long>(dis(gen)));
}

ValuePtr BSC::fn_type(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>("");

  const char *type_name = "";
  switch (args[0]->type) {
  case ValueType::NIL:
    type_name = "nil";
    break;
  case ValueType::BOOLEAN:
    type_name = "boolean";
    break;
  case ValueType::INTEGER:
    type_name = "integer";
    break;
  case ValueType::FLOAT:
    type_name = "float";
    break;
  case ValueType::STRING:
    type_name = "string";
    break;
  case ValueType::LIST:
    type_name = "list";
    break;
  case ValueType::MAP:
    type_name = "map";
    break;
  case ValueType::FUNCTION:
    type_name = "function";
    break;
  case ValueType::LAMBDA:
    type_name = "lambda";
    break;
  }

  return std::make_shared<Value>(type_name);
}

// Placeholder implementations for other BSC functions
ValuePtr BSC::fn_add(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>(0LL);
}
ValuePtr BSC::fn_sub(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>(0LL);
}
ValuePtr BSC::fn_mul(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>(0LL);
}
ValuePtr BSC::fn_div(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>(0.0);
}
ValuePtr BSC::fn_mod(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>(0LL);
}
ValuePtr BSC::fn_concat(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>("");
}
ValuePtr BSC::fn_substring(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>("");
}
ValuePtr BSC::fn_list(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_head(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_tail(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_nth(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_if(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_cond(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}
ValuePtr BSC::fn_print(const std::vector<ValuePtr> &args) {
  return std::make_shared<Value>();
}

ValuePtr BSC::fn_int(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0LL);

  if (args[0]->type == ValueType::INTEGER) {
    return args[0];
  } else if (args[0]->type == ValueType::FLOAT) {
    return std::make_shared<Value>(static_cast<long long>(args[0]->f));
  } else if (args[0]->type == ValueType::STRING) {
    try {
      long long val = std::stoll(args[0]->s);
      return std::make_shared<Value>(val);
    } catch (...) {
      return std::make_shared<Value>(0LL);
    }
  } else if (args[0]->type == ValueType::BOOLEAN) {
    return std::make_shared<Value>(args[0]->b ? 1LL : 0LL);
  }

  return std::make_shared<Value>(0LL);
}

ValuePtr BSC::fn_float(const std::vector<ValuePtr> &args) {
  if (args.empty())
    return std::make_shared<Value>(0.0);

  if (args[0]->type == ValueType::FLOAT) {
    return args[0];
  } else if (args[0]->type == ValueType::INTEGER) {
    return std::make_shared<Value>(static_cast<double>(args[0]->i));
  } else if (args[0]->type == ValueType::STRING) {
    try {
      double val = std::stod(args[0]->s);
      return std::make_shared<Value>(val);
    } catch (...) {
      return std::make_shared<Value>(0.0);
    }
  } else if (args[0]->type == ValueType::BOOLEAN) {
    return std::make_shared<Value>(args[0]->b ? 1.0 : 0.0);
  }

  return std::make_shared<Value>(0.0);
}

} // namespace jls
