#include "jls_shell.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace jls {

Shell::Shell() {}

void Shell::print_welcome() {
  std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║     JsonLambdaScript (JLS) Interactive Shell                  ║
║     A Lambda-based Language for JSON Manipulation             ║
╚═══════════════════════════════════════════════════════════════╝

Type 'help' for commands, 'exit' to quit.
)" << std::endl;
}

void Shell::print_help() {
  std::cout << R"(
Available Commands:
  help                - Show this help message
  exit, quit          - Exit the shell
  clear               - Clear screen
  tree <file>         - Display JSON file as a tree
  manage <library>    - Load a library (math, io, file, or custom)
  manage [library]    - List functions in a library
  
Supported JLS Syntax (BASIC-like):
  PRINT "hello"               - Print output
  LET x = 5                   - Variable assignment
  x = x + 1                   - Update variable
  IF x > 5 THEN ... END       - Conditional
  FOR i = 1 TO 10 ... NEXT    - Loop
  FUNCTION name(x, y) ... END - Define function
  CALL name(1, 2)             - Call function
  
Built-in Functions (BSC - Bvald Standard Collection):
  Math: ABS, SQRT, POW, FLOOR, CEIL, MIN, MAX, RANDOM
  String: LEN, STR
  I/O: INPUT
  Type: TYPE, INT, FLOAT

Available Libraries (MANAGE <library>):
  math - Trigonometry: SIN, COS, TAN, LN, LOG, EXP, ROUND, PI, E
  io   - I/O: PRINTNO, PAUSE
  file - Files: READ_FILE, WRITE_FILE, FILE_EXISTS
  jq   - JSON query: jq/run(filter, json_string)

Namespaced Calls:
  math/sin(1.0)       - Access a function inside a library
  jq/run(".foo", "{\"foo\": 1}")

Multiline Support:
  Use backslash (\) at end of line to continue on next line
  Or leave blocks incomplete (IF, FOR, FUNCTION) - shell detects automatically
  
Operators:
  +, -, *, /, %, ^            - Arithmetic
  =, <>, <, >, <=, >=         - Comparison
  AND, OR, NOT                - Logical
)" << std::endl;
}

bool Shell::is_incomplete_statement(const std::string &code) {
  // Check for line continuation character
  if (!code.empty() && code.back() == '\\') {
    return true;
  }

  // Simple heuristic: check for incomplete blocks
  std::string upper_code = code;
  std::transform(upper_code.begin(), upper_code.end(), upper_code.begin(),
                 ::toupper);

  // Count block keywords
  int if_count = 0, end_count = 0, for_count = 0, next_count = 0;
  int while_count = 0, function_count = 0;

  std::istringstream iss(upper_code);
  std::string word;
  while (iss >> word) {
    if (word == "IF")
      if_count++;
    else if (word == "FOR")
      for_count++;
    else if (word == "WHILE")
      while_count++;
    else if (word == "FUNCTION")
      function_count++;
    else if (word == "END")
      end_count++;
    else if (word == "NEXT")
      next_count++;
  }

  // Check if blocks are balanced
  int total_opens = if_count + while_count + function_count;
  int total_closes = end_count + next_count;

  return total_opens > total_closes;
}

void Shell::execute_tree_command(const std::string &arg) {
  std::string filename = arg;
  // Trim whitespace
  filename.erase(0, filename.find_first_not_of(" \t\n\r"));
  filename.erase(filename.find_last_not_of(" \t\n\r") + 1);

  if (filename.empty()) {
    std::cerr << "Usage: tree <filename>\n";
    return;
  }

  // Read file
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open file '" << filename << "'\n";
    return;
  }

  std::string json_text((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
  file.close();

  // Parse JSON
  JsonValue root;
  std::string err;
  if (!parse_json_dom(json_text, root, err)) {
    std::cerr << "Error parsing JSON: " << err << "\n";
    return;
  }

  // Print tree
  std::cout << "\nJSON Tree for: " << filename << "\n\n";
  print_json_tree(root, "", true);
  std::cout << std::endl;
}

void Shell::process_command(const std::string &line) {
  std::string trimmed = line;
  // Trim whitespace
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed.empty()) {
    return;
  }

  if (trimmed == "exit" || trimmed == "quit") {
    std::cout << "Goodbye!\n";
    exit(0);
  } else if (trimmed == "help") {
    print_help();
  } else if (trimmed == "clear") {
    // Clear screen (platform dependent)
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
  } else if (trimmed.substr(0, 4) == "tree" || trimmed.substr(0, 4) == "TREE") {
    // Extract filename after "tree "
    std::string arg = trimmed.substr(4);
    execute_tree_command(arg);
  } else if (trimmed.substr(0, 6) == "manage" ||
             trimmed.substr(0, 6) == "MANAGE") {
    // Extract library name after "manage "
    std::string arg = trimmed.substr(6);
    execute_manage_command(arg);
  } else {
    // Try to execute as JLS code
    execute_code(trimmed);
  }
}

void Shell::execute_code(const std::string &code) {
  try {
    // Tokenize
    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    // Parse
    Parser parser(tokens);
    auto ast = parser.parse();

    if (!parser.error_message().empty()) {
      std::cerr << "Parse Error: " << parser.error_message() << "\n";
      return;
    }

    // Evaluate
    auto result = evaluator.eval(ast);

    if (!evaluator.error_message().empty()) {
      std::cerr << "[STAT ERROR]: " << evaluator.error_message() << "\n";
      return;
    }

    // Print result only for expressions, not for statements that already print
    // Don't print results from PRINT, IF, FOR, WHILE, FUNCTION_DEF
    bool should_print_result = true;
    if (ast &&
        (ast->type == NodeType::PRINT || ast->type == NodeType::IF_STMT ||
         ast->type == NodeType::FOR_LOOP || ast->type == NodeType::WHILE_LOOP ||
         ast->type == NodeType::FUNCTION_DEF)) {
      should_print_result = false;
    }

    if (should_print_result && result && result->type != ValueType::NIL &&
        result->type != ValueType::FUNCTION) {
      switch (result->type) {
      case ValueType::BOOLEAN:
        std::cout << (result->b ? "true" : "false") << "\n";
        break;
      case ValueType::INTEGER:
        std::cout << result->i << "\n";
        break;
      case ValueType::FLOAT:
        std::cout << result->f << "\n";
        break;
      case ValueType::STRING:
        std::cout << result->s << "\n";
        break;
      case ValueType::LIST: {
        std::cout << "[";
        for (size_t i = 0; i < result->list.size(); ++i) {
          if (i > 0)
            std::cout << ", ";
          auto &item = result->list[i];
          if (item->type == ValueType::STRING) {
            std::cout << "\"" << item->s << "\"";
          } else if (item->type == ValueType::INTEGER) {
            std::cout << item->i;
          } else if (item->type == ValueType::FLOAT) {
            std::cout << item->f;
          } else if (item->type == ValueType::BOOLEAN) {
            std::cout << (item->b ? "true" : "false");
          }
        }
        std::cout << "]\n";
        break;
      }
      default:
        std::cout << "<value>\n";
        break;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Shell::execute_manage_command(const std::string &arg) {
  std::string libname = arg;
  // Trim whitespace
  libname.erase(0, libname.find_first_not_of(" \t\n\r"));
  libname.erase(libname.find_last_not_of(" \t\n\r") + 1);

  if (libname.empty()) {
    auto libs = LibraryLoader::get_available_libraries();
    std::cout << "Available libraries: ";
    for (size_t i = 0; i < libs.size(); ++i) {
      std::cout << libs[i];
      if (i + 1 < libs.size())
        std::cout << ", ";
    }
    std::cout << "\n";
    std::cout << "Usage: MANAGE <library> or MANAGE [library]\n";
    return;
  }

  if (libname.front() == '[' && libname.back() == ']') {
    std::string target = libname.substr(1, libname.size() - 2);
    if (!LibraryLoader::is_library_name(target)) {
      std::cerr << "Error: Unknown library '" << target << "'\n";
      return;
    }

    auto funcs = LibraryLoader::get_library_functions(target);
    if (funcs.empty()) {
      std::cout << "No functions exported by '" << target << "'.\n";
      return;
    }

    std::cout << "Functions in '" << target << "':\n";
    for (const auto &f : funcs) {
      std::cout << "  " << f << "\n";
    }
    std::cout << "Use as " << target << "/<function>()\n";
    return;
  }

  if (LibraryLoader::load_library(libname, evaluator.get_global_env())) {
    std::cout << "Library '" << libname << "' loaded successfully.\n";
  } else {
    std::cerr << "Error: Unknown library '" << libname << "'\n";
    auto libs = LibraryLoader::get_available_libraries();
    std::cout << "Available libraries: ";
    for (size_t i = 0; i < libs.size(); ++i) {
      std::cout << libs[i];
      if (i + 1 < libs.size())
        std::cout << ", ";
    }
    std::cout << "\n";
  }
}

void Shell::run() {
  print_welcome();

  std::string line;
  while (true) {
    // Show appropriate prompt
    if (multiline_buffer.empty()) {
      std::cout << prompt;
    } else {
      std::cout << continuation_prompt;
    }

    std::getline(std::cin, line);

    if (std::cin.eof()) {
      std::cout << "\nGoodbye!\n";
      break;
    }

    // Trim whitespace
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    // Handle line continuation
    if (!trimmed.empty() && trimmed.back() == '\\') {
      // Remove backslash and add to buffer
      trimmed.pop_back();
      multiline_buffer.push_back(trimmed);
      continue;
    }

    // Add current line to buffer
    if (!trimmed.empty() || !multiline_buffer.empty()) {
      multiline_buffer.push_back(trimmed);
    }

    // Check if we have a complete statement
    std::string full_code;
    for (const auto &l : multiline_buffer) {
      full_code += l + " ";
    }

    if (!full_code.empty() && is_incomplete_statement(full_code)) {
      // Continue collecting lines
      continue;
    }

    // Execute complete statement
    if (!full_code.empty()) {
      process_command(full_code);
    }

    // Clear buffer
    multiline_buffer.clear();
  }
}

} // namespace jls
