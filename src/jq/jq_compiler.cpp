#include "jq_compiler.hpp"
#include "jq_bytecode.hpp"
#include "jq_parser.hpp"

namespace jq {

Compiler::Compiler() = default;

bool Compiler::compile(const ASTNodePtr &ast, Program &program,
                       std::string &err) {
  program.code.clear();
  program.pool.strings.clear();
  program.pool.numbers.clear();

  if (!emit_node(ast, program, err))
    return false;

  return program.validate(err);
}

bool Compiler::emit_node(const ASTNodePtr &node, Program &prog,
                         std::string &err) {
  if (!node) {
    err = "Null AST node";
    return false;
  }

  switch (node->type) {
  case NodeType::IDENTITY:
    prog.code.push_back({OpCode::LOAD_IDENTITY, -1, -1});
    return true;
  case NodeType::FIELD: {
    int sid = prog.pool.add_string(node->name);
    prog.code.push_back({OpCode::GET_FIELD, sid, -1});
    return true;
  }
  case NodeType::INDEX: {
    if (node->children.empty()) {
      err = "Index node missing child";
      return false;
    }
    auto idx = node->children[0];
    if (idx->type == NodeType::LITERAL && idx->literal) {
      if (idx->literal->is_number()) {
        int nid = prog.pool.add_number(idx->literal->n);
        prog.code.push_back({OpCode::GET_INDEX_NUM, nid, -1});
        return true;
      }
      if (idx->literal->is_string()) {
        int sid = prog.pool.add_string(idx->literal->s);
        prog.code.push_back({OpCode::GET_INDEX_STR, sid, -1});
        return true;
      }
    }
    err = "Unsupported index expression";
    return false;
  }
  case NodeType::ITERATOR:
    prog.code.push_back({OpCode::ITERATE, -1, -1});
    return true;
  case NodeType::PIPE: {
    if (node->children.size() != 2) {
      err = "Pipe expects 2 children";
      return false;
    }
    if (!emit_node(node->children[0], prog, err))
      return false;
    return emit_node(node->children[1], prog, err);
  }
  case NodeType::BINARY_OP: {
    if (node->op == "+" && node->children.size() == 2) {
      // compile left then add const if right is number literal
      if (!emit_node(node->children[0], prog, err))
        return false;
      auto rhs = node->children[1];
      if (rhs && rhs->type == NodeType::LITERAL && rhs->literal &&
          rhs->literal->is_number()) {
        int nid = prog.pool.add_number(rhs->literal->n);
        prog.code.push_back({OpCode::ADD_CONST, nid, -1});
        return true;
      }
    }
    err = "Unsupported binary op";
    return false;
  }
  case NodeType::FUNCTION_CALL: {
    // Emit opcodes for any receiver/arguments, then emit builtin call
    // For now, builtins are 0-arg and operate on current value
    int sid = prog.pool.add_string(node->name);
    prog.code.push_back({OpCode::BUILTIN_CALL, sid, -1});
    return true;
  }
  default:
    err = "Unsupported AST node type";
    return false;
  }
}

} // namespace jq
