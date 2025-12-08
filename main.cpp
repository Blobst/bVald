#include "./include/libjsonval.hpp"

int main(int argc, char *argv[]) {
  if (argc == 1) {
    print_help(argv[0]);
    return 1;
  }

  std::string filename;
  std::string schema_arg;
  bool use_schema = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      print_help(argv[0]);
      return 0;
    }
    if (arg == "-v" || arg == "--version") {
      std::cout << VERSION << std::endl;
      return 0;
    }
    if (arg == "-f" || arg == "--file") {
      if (i + 1 < argc) {
        filename = argv[++i];
      } else {
        std::cerr << "Error: -f requires a filename\n";
        return 1;
      }
      continue;
    }
    if (arg == "-s" || arg == "--schema") {
      if (i + 1 < argc) {
        schema_arg = argv[++i];
      } else {
        std::cerr << "Error: -s requires a schema id/url\n";
        return 1;
      }
      continue;
    }
    if (arg == "-us" || arg == "--use-schema") {
      use_schema = true;
      continue;
    }
    // treat first non-option as filename
    if (filename.empty() && !arg.empty() && arg[0] != '-') {
      filename = arg;
    }
  }

  if (filename.empty() && schema_arg.empty()) {
    std::cerr << "Error: missing filename\n";
    print_help(argv[0]);
    return 1;
  }

  std::string content;
  if (!filename.empty()) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
      std::cerr << "Error: cannot open file '" << filename << "'\n";
      return 1;
    }
    content.assign((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  }

  std::string error;
  // If a schema argument is provided, attempt to fetch it and print some info
  if (!schema_arg.empty()) {
    std::string cerr;
    if (!init_schema_registry("schemas.json", cerr)) {
      std::cerr << "Warning: unable to load schemas.json: " << cerr << "\n";
    }
    // schema registry initialized
    std::string schema_content;
    if (get_schema_source(schema_arg, schema_content, cerr)) {
      std::cout << "Fetched schema (length=" << schema_content.size() << ")\n";
      // try resolve linked schemas as well
      std::map<std::string, std::string> resolved;
      if (resolve_schema_links(schema_arg, resolved, cerr)) {
        std::cout << "Resolved " << resolved.size() << " linked schemas\n";
      }
      if (filename.empty())
        return 0; // only fetching, don't validate
    } else {
      std::cerr << "Failed to fetch schema: " << cerr << "\n";
      if (filename.empty())
        return 2;
    }
  }
  // If use_schema is set, attempt to obtain the schema to use and validate
  // against it using a minimal schema validator subset.
  if (use_schema) {
    if (filename.empty()) {
      std::cerr << "Error: --use-schema requires a filename to validate\n";
      return 1;
    }
    std::string cerr;
    if (!init_schema_registry("schemas.json", cerr)) { /* ignore */
    }
    std::string schema_content;
    std::string selected_schema = schema_arg;
    // if no schema provided, try find "$schema" in the file content
    if (selected_schema.empty()) {
      // quick detection: look for "\"$schema\"\s*:\s*\"(url or id)\""
      size_t pos = content.find("\"$schema\"");
      if (pos != std::string::npos) {
        size_t col = content.find(':', pos);
        if (col != std::string::npos) {
          size_t q1 = content.find('"', col);
          size_t q2 = content.find('"', q1 + 1);
          if (q1 != std::string::npos && q2 != std::string::npos)
            selected_schema = content.substr(q1 + 1, q2 - q1 - 1);
        }
      }
    }
    if (selected_schema.empty()) {
      std::cerr
          << "Error: no schema specified (use -s or include $schema in file)\n";
      return 1;
    }
    if (!get_schema_source(selected_schema, schema_content, cerr)) {
      std::cerr << "Error: cannot load schema: " << cerr << "\n";
      return 1;
    }
    // optionally resolve linked schemas; not used yet
    std::map<std::string, std::string> resolved;
    resolve_schema_links(selected_schema, resolved, cerr);

    std::string verr;
    if (!validate_json_with_schema(content, schema_content, verr)) {
      std::cerr << "Schema validation failed: " << verr << "\n";
      return 2;
    }
    std::cout << "OK: valid against schema\n";
    return 0;
  }
  bool ok = validate_json(content, error);
  if (ok) {
    std::cout << "OK: valid JSON\n";
    return 0;
  } else {
    std::cerr << "Invalid JSON: " << error << "\n";
    return 2;
  }
}