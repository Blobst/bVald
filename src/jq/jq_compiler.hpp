#ifndef JQ_COMPILER_HPP
#define JQ_COMPILER_HPP

#include "jq_parser.hpp"
#include <string>

namespace jq {

struct Program;

class Compiler {
public:
  Compiler();
  bool compile(const ASTNodePtr &ast, Program &program, std::string &err);

private:
  bool emit_node(const ASTNodePtr &node, Program &program, std::string &err);
};

} // namespace jq

#endif // JQ_COMPILER_HPP