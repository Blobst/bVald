#include "jq_lexer.hpp"
#include <cctype>
#include <sstream>

namespace jq {

Lexer::Lexer(const std::string &input)
    : input(input), pos(0), line(1), column(1) {}

char Lexer::current() const {
  if (pos >= input.size())
    return '\0';
  return input[pos];
}

char Lexer::peek(size_t offset) const {
  if (pos + offset >= input.size())
    return '\0';
  return input[pos + offset];
}

void Lexer::advance() {
  if (pos < input.size()) {
    if (input[pos] == '\n') {
      ++line;
      column = 1;
    } else {
      ++column;
    }
    ++pos;
  }
}

void Lexer::skip_whitespace() {
  while (std::isspace(static_cast<unsigned char>(current()))) {
    advance();
  }
}

void Lexer::skip_comment() {
  if (current() == '#') {
    while (current() != '\n' && current() != '\0') {
      advance();
    }
  }
}

Token Lexer::read_number() {
  size_t start_line = line;
  size_t start_col = column;
  std::string num;

  if (current() == '-') {
    num += current();
    advance();
  }

  while (std::isdigit(static_cast<unsigned char>(current()))) {
    num += current();
    advance();
  }

  if (current() == '.') {
    num += current();
    advance();
    while (std::isdigit(static_cast<unsigned char>(current()))) {
      num += current();
      advance();
    }
  }

  if (current() == 'e' || current() == 'E') {
    num += current();
    advance();
    if (current() == '+' || current() == '-') {
      num += current();
      advance();
    }
    while (std::isdigit(static_cast<unsigned char>(current()))) {
      num += current();
      advance();
    }
  }

  return Token(TokenType::NUMBER, num, start_line, start_col);
}

Token Lexer::read_string() {
  size_t start_line = line;
  size_t start_col = column;
  advance(); // Skip opening quote

  std::string str;
  while (current() != '"' && current() != '\0') {
    if (current() == '\\') {
      advance();
      switch (current()) {
      case 'n':
        str += '\n';
        break;
      case 't':
        str += '\t';
        break;
      case 'r':
        str += '\r';
        break;
      case '\\':
        str += '\\';
        break;
      case '"':
        str += '"';
        break;
      case '/':
        str += '/';
        break;
      case 'b':
        str += '\b';
        break;
      case 'f':
        str += '\f';
        break;
      default:
        str += current();
        break;
      }
      advance();
    } else {
      str += current();
      advance();
    }
  }

  if (current() == '"')
    advance();

  return Token(TokenType::STRING, str, start_line, start_col);
}

Token Lexer::read_identifier() {
  size_t start_line = line;
  size_t start_col = column;
  std::string id;

  while (std::isalnum(static_cast<unsigned char>(current())) ||
         current() == '_' || current() == '$') {
    id += current();
    advance();
  }

  // Check for keywords
  if (id == "true")
    return Token(TokenType::TRUE, id, start_line, start_col);
  if (id == "false")
    return Token(TokenType::FALSE, id, start_line, start_col);
  if (id == "null")
    return Token(TokenType::NULL_VALUE, id, start_line, start_col);
  if (id == "and")
    return Token(TokenType::AND, id, start_line, start_col);
  if (id == "or")
    return Token(TokenType::OR, id, start_line, start_col);
  if (id == "not")
    return Token(TokenType::NOT, id, start_line, start_col);

  return Token(TokenType::IDENTIFIER, id, start_line, start_col);
}

Token Lexer::next_token() {
  skip_whitespace();

  while (current() == '#') {
    skip_comment();
    skip_whitespace();
  }

  size_t tok_line = line;
  size_t tok_col = column;
  char ch = current();

  if (ch == '\0')
    return Token(TokenType::EOF_TOKEN, "", tok_line, tok_col);

  // Numbers
  if (std::isdigit(static_cast<unsigned char>(ch)) ||
      (ch == '-' && std::isdigit(static_cast<unsigned char>(peek())))) {
    return read_number();
  }

  // Strings
  if (ch == '"') {
    return read_string();
  }

  // Identifiers
  if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_' || ch == '$') {
    return read_identifier();
  }

  // Two-character operators
  if (ch == '=' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::EQ, "==", tok_line, tok_col);
  }
  if (ch == '!' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::NE, "!=", tok_line, tok_col);
  }
  if (ch == '<' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::LE, "<=", tok_line, tok_col);
  }
  if (ch == '>' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::GE, ">=", tok_line, tok_col);
  }
  if (ch == '|' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::UPDATE, "|=", tok_line, tok_col);
  }
  if (ch == '+' && peek() == '=') {
    advance();
    advance();
    return Token(TokenType::PLUS_ASSIGN, "+=", tok_line, tok_col);
  }
  if (ch == '/' && peek() == '/') {
    advance();
    advance();
    return Token(TokenType::DOUBLE_SLASH, "//", tok_line, tok_col);
  }
  if (ch == '.' && peek() == '.') {
    advance();
    advance();
    return Token(TokenType::RECURSIVE, "..", tok_line, tok_col);
  }

  // Single-character tokens
  advance();
  switch (ch) {
  case '.':
    return Token(TokenType::DOT, ".", tok_line, tok_col);
  case '|':
    return Token(TokenType::PIPE, "|", tok_line, tok_col);
  case ',':
    return Token(TokenType::COMMA, ",", tok_line, tok_col);
  case ';':
    return Token(TokenType::SEMICOLON, ";", tok_line, tok_col);
  case ':':
    return Token(TokenType::COLON, ":", tok_line, tok_col);
  case '+':
    return Token(TokenType::PLUS, "+", tok_line, tok_col);
  case '-':
    return Token(TokenType::MINUS, "-", tok_line, tok_col);
  case '*':
    return Token(TokenType::STAR, "*", tok_line, tok_col);
  case '/':
    return Token(TokenType::SLASH, "/", tok_line, tok_col);
  case '%':
    return Token(TokenType::PERCENT, "%", tok_line, tok_col);
  case '=':
    return Token(TokenType::ASSIGN, "=", tok_line, tok_col);
  case '<':
    return Token(TokenType::LT, "<", tok_line, tok_col);
  case '>':
    return Token(TokenType::GT, ">", tok_line, tok_col);
  case '(':
    return Token(TokenType::LPAREN, "(", tok_line, tok_col);
  case ')':
    return Token(TokenType::RPAREN, ")", tok_line, tok_col);
  case '[':
    return Token(TokenType::LBRACKET, "[", tok_line, tok_col);
  case ']':
    return Token(TokenType::RBRACKET, "]", tok_line, tok_col);
  case '{':
    return Token(TokenType::LBRACE, "{", tok_line, tok_col);
  case '}':
    return Token(TokenType::RBRACE, "}", tok_line, tok_col);
  case '?':
    return Token(TokenType::QUESTION, "?", tok_line, tok_col);
  default:
    return Token(TokenType::ERROR, std::string(1, ch), tok_line, tok_col);
  }
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  Token tok;

  do {
    tok = next_token();
    tokens.push_back(tok);
  } while (tok.type != TokenType::EOF_TOKEN && tok.type != TokenType::ERROR);

  return tokens;
}

} // namespace jq
