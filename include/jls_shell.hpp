#ifndef JLS_SHELL_HPP
#define JLS_SHELL_HPP

#include "jls.hpp"
#include "jls_library.hpp"
#include "libjsonval.hpp"
#include <string>
#include <vector>

namespace jls {

class Shell {
public:
  Shell();
  void run();

private:
  Evaluator evaluator;
  std::string prompt = "jls> ";
  std::string continuation_prompt = "...> ";
  std::vector<std::string> multiline_buffer;

  void print_welcome();
  void print_help();
  void process_command(const std::string &line);
  void execute_code(const std::string &code);
  void execute_tree_command(const std::string &arg);
  void execute_manage_command(const std::string &arg);
  bool is_incomplete_statement(const std::string &code);
};

} // namespace jls

#endif // JLS_SHELL_HPP
