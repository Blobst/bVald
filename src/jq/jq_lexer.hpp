#ifndef JQ_LEXER_HPP
#define JQ_LEXER_HPP

#include <string>
#include <vector>

namespace jq {

// Token types for jq filters
enum class TokenType {
  // Literals
  NUMBER,
  STRING,
  TRUE,
  FALSE,
  NULL_VALUE,

  // Identifiers and keywords
  IDENTIFIER,

  // Operators
  DOT,       // .
  PIPE,      // |
  COMMA,     // ,
  SEMICOLON, // ;
  COLON,     // :

  // Arithmetic
  PLUS,    // +
  MINUS,   // -
  STAR,    // *
  SLASH,   // /
  PERCENT, // %

  // Comparison
  EQ, // ==
  NE, // !=
  LT, // <
  LE, // <=
  GT, // >
  GE, // >=

  // Logic
  AND, // and
  OR,  // or
  NOT, // not

  // Assignment
  ASSIGN,      // =
  UPDATE,      // |=
  PLUS_ASSIGN, // +=

  // Delimiters
  LPAREN,   // (
  RPAREN,   // )
  LBRACKET, // [
  RBRACKET, // ]
  LBRACE,   // {
  RBRACE,   // }

  // Special
  QUESTION,     // ?
  DOUBLE_SLASH, // //
  RECURSIVE,    // ..

  // End
  EOF_TOKEN,
  ERROR
};

struct Token {
  TokenType type;
  std::string value;
  size_t line;
  size_t column;

  Token(TokenType t = TokenType::ERROR, const std::string &v = "", size_t l = 0,
        size_t c = 0)
      : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
  explicit Lexer(const std::string &input);

  std::vector<Token> tokenize();
  Token next_token();

private:
  std::string input;
  size_t pos;
  size_t line;
  size_t column;

  char current() const;
  char peek(size_t offset = 1) const;
  void advance();
  void skip_whitespace();
  void skip_comment();

  Token read_number();
  Token read_string();
  Token read_identifier();
};

} // namespace jq

#endif // JQ_LEXER_HPP
