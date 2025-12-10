#include "libjsonval.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#ifdef _WIN32
#include <direct.h> // _mkdir
#include <io.h>     // _access
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static bool dir_exists(const std::string &path) {
#ifdef _WIN32
  return _access(path.c_str(), 0) == 0;
#else
  return access(path.c_str(), F_OK) == 0;
#endif
}
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

const std::string VERSION =
    "0.1.4"; // i was to lazy to make a git repo by 0.1.0

// ================= Helper Functions for Error Messages =================
// Calculate Levenshtein distance for typo suggestions
static size_t levenshtein_distance(const std::string &a, const std::string &b) {
  size_t m = a.size();
  size_t n = b.size();
  std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

  for (size_t i = 0; i <= m; ++i)
    dp[i][0] = i;
  for (size_t j = 0; j <= n; ++j)
    dp[0][j] = j;

  for (size_t i = 1; i <= m; ++i) {
    for (size_t j = 1; j <= n; ++j) {
      if (a[i - 1] == b[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1];
      } else {
        dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
      }
    }
  }
  return dp[m][n];
}

// Find closest match from a list of candidates
static std::string
find_closest_match(const std::string &typo,
                   const std::vector<std::string> &candidates,
                   size_t max_distance = 3) {
  if (candidates.empty())
    return "";
  size_t min_dist = max_distance + 1;
  std::string best_match;
  for (const auto &candidate : candidates) {
    size_t dist = levenshtein_distance(typo, candidate);
    if (dist < min_dist) {
      min_dist = dist;
      best_match = candidate;
    }
  }
  return (min_dist <= max_distance) ? best_match : "";
}

struct JsonParser {
  const std::string &s;
  size_t i = 0;
  std::string err;
  size_t line = 1;
  size_t column = 1;

  JsonParser(const std::string &str) : s(str), i(0), line(1), column(1) {}

  void skip_ws() {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
      if (s[i] == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
      ++i;
    }
  }

  bool parse_value() {
    skip_ws();
    if (i >= s.size()) {
      err = "unexpected end of input at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
    char c = s[i];
    if (c == '{')
      return parse_object();
    if (c == '[')
      return parse_array();
    if (c == '"')
      return parse_string();
    if (c == '-' || (c >= '0' && c <= '9'))
      return parse_number();
    if (c == 't' || c == 'f' || c == 'n')
      return parse_literal();
    err = std::string("unexpected character '") + c + "' at line " +
          std::to_string(line) + ", column " + std::to_string(column);
    return false;
  }

  bool parse_object() {
    if (s[i] != '{') {
      err = "expected '{' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    ++column;
    ++i;
    skip_ws();
    if (i < s.size() && s[i] == '}') {
      ++column;
      ++i;
      return true;
    }
    while (true) {
      skip_ws();
      if (!parse_string())
        return false;
      skip_ws();
      if (i >= s.size() || s[i] != ':') {
        err = "expected ':' after object key at line " + std::to_string(line) +
              ", column " + std::to_string(column);
        return false;
      }
      ++column;
      ++i;
      if (!parse_value())
        return false;
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++column;
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == '}') {
        ++column;
        ++i;
        return true;
      }
      err = "expected ',' or '}' in object at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
  }

  bool parse_array() {
    if (s[i] != '[') {
      err = "expected '[' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    ++column;
    ++i;
    skip_ws();
    if (i < s.size() && s[i] == ']') {
      ++column;
      ++i;
      return true;
    }
    while (true) {
      if (!parse_value())
        return false;
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++column;
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == ']') {
        ++column;
        ++i;
        return true;
      }
      err = "expected ',' or ']' in array at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
  }

  bool parse_string() {
    if (s[i] != '"') {
      err = "expected '\"' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    ++column;
    ++i;
    while (i < s.size()) {
      char c = s[i++];
      if (c == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
      if (c == '"')
        return true;
      if (c == '\\') {
        if (i >= s.size()) {
          err = "unterminated escape in string at line " +
                std::to_string(line) + ", column " + std::to_string(column);
          return false;
        }
        char e = s[i++];
        ++column;
        if (e == 'u') {
          for (int k = 0; k < 4; ++k) {
            if (i >= s.size() || !is_hex(s[i++])) {
              err = "invalid unicode escape in string at line " +
                    std::to_string(line) + ", column " + std::to_string(column);
              return false;
            }
            ++column;
          }
        } else {
          if (e != '"' && e != '\\' && e != '/' && e != 'b' && e != 'f' &&
              e != 'n' && e != 'r' && e != 't') {
            err = std::string("invalid escape: ") + e + " at line " +
                  std::to_string(line) + ", column " + std::to_string(column);
            return false;
          }
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        err = "control character in string at line " + std::to_string(line) +
              ", column " + std::to_string(column);
        return false;
      }
    }
    err = "unterminated string at line " + std::to_string(line) + ", column " +
          std::to_string(column);
    return false;
  }

  static bool is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
  }

  bool parse_number() {
    size_t start = i;
    if (s[i] == '-') {
      ++column;
      ++i;
    }
    if (i >= s.size()) {
      err = "invalid number at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    if (s[i] == '0') {
      ++column;
      ++i;
    } else if (s[i] >= '1' && s[i] <= '9') {
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) {
        ++column;
        ++i;
      }
    } else {
      err = "invalid number at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    if (i < s.size() && s[i] == '.') {
      ++column;
      ++i;
      if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) {
        err = "invalid fractional part in number at line " +
              std::to_string(line) + ", column " + std::to_string(column);
        return false;
      }
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) {
        ++column;
        ++i;
      }
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
      ++column;
      ++i;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        ++column;
        ++i;
      }
      if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) {
        err = "invalid exponent in number at line " + std::to_string(line) +
              ", column " + std::to_string(column);
        return false;
      }
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i]))) {
        ++column;
        ++i;
      }
    }
    return i > start;
  }

  bool parse_literal() {
    if (s.compare(i, 4, "true") == 0) {
      i += 4;
      column += 4;
      return true;
    }
    if (s.compare(i, 5, "false") == 0) {
      i += 5;
      column += 5;
      return true;
    }
    if (s.compare(i, 4, "null") == 0) {
      i += 4;
      column += 4;
      return true;
    }
    err = "invalid literal at line " + std::to_string(line) + ", column " +
          std::to_string(column);
    return false;
  }
};

// Minimal DOM value for use by schema validator
struct JsonValue {
  enum Type { T_NULL, T_BOOL, T_NUMBER, T_STRING, T_OBJECT, T_ARRAY } t;
  bool b = false;
  double n = 0.0;
  std::string s;
  std::map<std::string, JsonValue> o;
  std::vector<JsonValue> a;
};

// Simple JSON DOM parser (builds JsonValue tree)
struct JsonDOMParser {
  const std::string &s;
  size_t i = 0;
  std::string err;
  size_t line = 1;
  size_t column = 1;

  JsonDOMParser(const std::string &str) : s(str), i(0), line(1), column(1) {}

  void skip_ws() {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
      if (s[i] == '\n') {
        ++line;
        column = 1;
      } else {
        ++column;
      }
      ++i;
    }
  }
  bool parse_value(JsonValue &out) {
    skip_ws();
    if (i >= s.size()) {
      err = "unexpected end of input at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
    char c = s[i];
    if (c == '{')
      return parse_object(out);
    if (c == '[')
      return parse_array(out);
    if (c == '"')
      return parse_string(out);
    if (c == 't' || c == 'f' || c == 'n')
      return parse_literal(out);
    if (c == '-' || (c >= '0' && c <= '9'))
      return parse_number(out);
    err = std::string("unexpected character '") + c + "' at line " +
          std::to_string(line) + ", column " + std::to_string(column);
    return false;
  }
  bool parse_object(JsonValue &out) {
    if (s[i] != '{') {
      err = "expected '{' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    ++column;
    ++i;
    skip_ws();
    out.t = JsonValue::T_OBJECT;
    if (i < s.size() && s[i] == '}') {
      ++column;
      ++i;
      return true;
    }
    while (true) {
      skip_ws();
      JsonValue keyv;
      if (!parse_string(keyv))
        return false;
      skip_ws();
      if (i >= s.size() || s[i] != ':') {
        err = "expected ':' after object key at line " + std::to_string(line) +
              ", column " + std::to_string(column);
        return false;
      }
      ++column;
      ++i;
      skip_ws();
      JsonValue val;
      if (!parse_value(val))
        return false;
      out.o[keyv.s] = val;
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++column;
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == '}') {
        ++column;
        ++i;
        break;
      }
      err = "expected ',' or '}' in object at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
    return true;
  }
  bool parse_array(JsonValue &out) {
    if (s[i] != '[') {
      err = "expected '[' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    ++column;
    ++i;
    skip_ws();
    out.t = JsonValue::T_ARRAY;
    if (i < s.size() && s[i] == ']') {
      ++column;
      ++i;
      return true;
    }
    while (true) {
      skip_ws();
      JsonValue elem;
      if (!parse_value(elem))
        return false;
      out.a.push_back(elem);
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++column;
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == ']') {
        ++column;
        ++i;
        break;
      }
      err = "expected ',' or ']' in array at line " + std::to_string(line) +
            ", column " + std::to_string(column);
      return false;
    }
    return true;
  }
  bool parse_string(JsonValue &out) {
    if (s[i] != '"') {
      err = "expected '\"' at line " + std::to_string(line) + ", column " +
            std::to_string(column);
      return false;
    }
    size_t j = i + 1;
    size_t temp_line = line;
    size_t temp_column = column + 1;
    std::string collected;
    while (j < s.size()) {
      char c = s[j];
      if (c == '\n') {
        ++temp_line;
        temp_column = 1;
      } else {
        ++temp_column;
      }
      if (c == '\\') {
        if (j + 1 < s.size()) {
          collected += s[j + 1];
          j += 2;
          if (s[j - 1] == '\n') {
            ++temp_line;
            temp_column = 1;
          } else {
            ++temp_column;
          }
          continue;
        }
        err = "unfinished escape in string at line " +
              std::to_string(temp_line) + ", column " +
              std::to_string(temp_column);
        return false;
      }
      if (c == '"') {
        line = temp_line;
        column = temp_column;
        break;
      }
      collected += c;
      ++j;
    }
    if (j >= s.size()) {
      err = "unterminated string at line " + std::to_string(temp_line) +
            ", column " + std::to_string(temp_column);
      return false;
    }
    out.t = JsonValue::T_STRING;
    out.s = collected;
    i = j + 1;
    return true;
  }
  bool parse_number(JsonValue &out) {
    size_t j = i;
    if (s[j] == '-')
      ++j;
    while (j < s.size() && isdigit((unsigned char)s[j]))
      ++j;
    if (j < s.size() && s[j] == '.') {
      ++j;
      while (j < s.size() && isdigit((unsigned char)s[j]))
        ++j;
    }
    std::string t = s.substr(i, j - i);
    if (!t.empty()) {
      char *endp = nullptr;
      out.n = std::strtod(t.c_str(), &endp);
      // if endp == t.c_str(), conversion failed; keep n as 0
    }
    out.t = JsonValue::T_NUMBER;
    column += (j - i);
    i = j;
    return true;
  }
  bool parse_literal(JsonValue &out) {
    if (s.compare(i, 4, "true") == 0) {
      out.t = JsonValue::T_BOOL;
      out.b = true;
      i += 4;
      column += 4;
      return true;
    }
    if (s.compare(i, 5, "false") == 0) {
      out.t = JsonValue::T_BOOL;
      out.b = false;
      i += 5;
      column += 5;
      return true;
    }
    if (s.compare(i, 4, "null") == 0) {
      out.t = JsonValue::T_NULL;
      i += 4;
      column += 4;
      return true;
    }
    err = "invalid literal at line " + std::to_string(line) + ", column " +
          std::to_string(column);
    return false;
  }
};

static bool parse_json_dom(const std::string &text, JsonValue &out,
                           std::string &err) {
  JsonDOMParser p(text);
  if (!p.parse_value(out)) {
    err = p.err;
    return false;
  }
  p.skip_ws();
  if (p.i != text.size()) {
    err = "trailing data after JSON value";
    return false;
  }
  return true;
}

// Minimal schema validator support: supports 'type', 'required',
// 'properties', 'items', and 'enum'. The schema passed here is expected as
// a parsed JSON object (JsonValue tree).
static std::string type_name(const JsonValue &v) {
  switch (v.t) {
  case JsonValue::T_NULL:
    return "null";
  case JsonValue::T_BOOL:
    return "boolean";
  case JsonValue::T_NUMBER:
    return "number";
  case JsonValue::T_STRING:
    return "string";
  case JsonValue::T_OBJECT:
    return "object";
  case JsonValue::T_ARRAY:
    return "array";
  }
  return "unknown";
}

// Check for typos in schema properties and suggest corrections
static void
suggest_property(const std::string &invalid_key,
                 const std::map<std::string, JsonValue> &valid_props,
                 std::string &suggestion) {
  std::vector<std::string> candidates;
  for (const auto &p : valid_props) {
    candidates.push_back(p.first);
  }
  suggestion = find_closest_match(invalid_key, candidates);
}

static bool
validate_schema_rec(const JsonValue &data, const JsonValue &schema,
                    const std::map<std::string, JsonValue> &resolved,
                    std::string &err, const std::string &path = "") {
  // type
  if (schema.t == JsonValue::T_OBJECT) {
    auto it_type = schema.o.find("type");
    if (it_type != schema.o.end() && it_type->second.t == JsonValue::T_STRING) {
      std::string st = it_type->second.s;
      const char *dtype = nullptr;
      if (st == "object")
        dtype = "object";
      else if (st == "array")
        dtype = "array";
      else if (st == "string")
        dtype = "string";
      else if (st == "number")
        dtype = "number";
      else if (st == "boolean")
        dtype = "boolean";
      else if (st == "null")
        dtype = "null";
      if (dtype) {
        if (std::string(dtype) != type_name(data)) {
          err = "type mismatch at '" + path + "', expected '" + st + "' got '" +
                type_name(data) + "'";
          return false;
        }
      }
    }

    // required
    auto it_req = schema.o.find("required");
    if (it_req != schema.o.end() && it_req->second.t == JsonValue::T_ARRAY) {
      if (data.t != JsonValue::T_OBJECT) {
        err = "expected object at '" + path + "' for required properties";
        return false;
      }
      for (const auto &rq : it_req->second.a) {
        if (rq.t == JsonValue::T_STRING) {
          if (data.o.find(rq.s) == data.o.end()) {
            err = "missing required property '" + rq.s + "' at '" + path + "'";
            return false;
          }
        }
      }
    }

    // properties
    auto it_props = schema.o.find("properties");
    if (it_props != schema.o.end() &&
        it_props->second.t == JsonValue::T_OBJECT) {
      if (data.t != JsonValue::T_OBJECT) {
        err = "expected object at '" + path + "' for properties";
        return false;
      }
      for (const auto &p : it_props->second.o) {
        auto it_data = data.o.find(p.first);
        if (it_data != data.o.end()) {
          std::string subpath = path.empty() ? p.first : (path + "." + p.first);
          if (!validate_schema_rec(it_data->second, p.second, resolved, err,
                                   subpath))
            return false;
        }
      }
      // Check for unknown properties in data and suggest corrections
      for (const auto &data_prop : data.o) {
        if (it_props->second.o.find(data_prop.first) ==
            it_props->second.o.end()) {
          std::string suggestion;
          suggest_property(data_prop.first, it_props->second.o, suggestion);
          err = "unknown property '" + data_prop.first + "' at '" + path + "'";
          if (!suggestion.empty()) {
            err += ". Did you mean '" + suggestion + "'?";
          }
          return false;
        }
      }
    }

    // enum
    auto it_enum = schema.o.find("enum");
    if (it_enum != schema.o.end() && it_enum->second.t == JsonValue::T_ARRAY) {
      bool match = false;
      for (const auto &e : it_enum->second.a) {
        // only compare strings and numbers for now
        if (e.t == data.t) {
          if (e.t == JsonValue::T_STRING && data.t == JsonValue::T_STRING &&
              e.s == data.s)
            match = true;
          else if (e.t == JsonValue::T_NUMBER &&
                   data.t == JsonValue::T_NUMBER && e.n == data.n)
            match = true;
        }
      }
      if (!match) {
        err = "enum mismatch at '" + path + "'";
        return false;
      }
    }
  }

  // array items
  if (schema.t == JsonValue::T_OBJECT) {
    auto it_items = schema.o.find("items");
    if (it_items != schema.o.end()) {
      if (data.t != JsonValue::T_ARRAY) {
        err = "expected array at '" + path + "' for items";
        return false;
      }
      for (size_t i = 0; i < data.a.size(); ++i) {
        std::string subpath = path + "[" + std::to_string(i) + "]";
        if (!validate_schema_rec(data.a[i], it_items->second, resolved, err,
                                 subpath))
          return false;
      }
    }
  }

  return true;
}

bool validate_json_with_schema(const std::string &json_text,
                               const std::string &schema_text,
                               std::string &err) {
  JsonValue data;
  if (!parse_json_dom(json_text, data, err))
    return false;
  JsonValue schema;
  if (!parse_json_dom(schema_text, schema, err))
    return false;
  std::map<std::string, JsonValue>
      resolved; // placeholder for future $ref handling
  if (!validate_schema_rec(data, schema, resolved, err))
    return false;
  return true;
}

bool validate_json(const std::string &text, std::string &error_msg) {
  JsonParser p(text);
  p.skip_ws();
  if (!p.parse_value()) {
    error_msg = p.err;
    return false;
  }
  // done.
  p.skip_ws();
  if (p.i != text.size()) {
    error_msg = "trailing data after JSON value";
    return false;
  }
  return true;
}

void print_help(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options] <filename>\n"
            << "Options:\n"
            << "  -h, --help     Show this help message\n"
            << "  -v, --version  Show version information\n"
            << "  -f, --file <filename>  Specify input file\n"
            << "  -s, --schema <id|url>   Fetch a schema by id or URL and "
               "print info\n"
            << "  -us, --use-schema        Validate file using specified or "
               "embedded $schema\n";
}

// Simple schema registry implementation (parsing `schemas.json` in a robust
// way is out of scope; we accept a lightweight parser that extracts
// `schemas[].id` and `schemas[].source` fields.)

static std::vector<SchemaEntry> g_schema_registry;
static bool g_resolve_remote = true;
static std::string g_cache_dir;

static std::string trim_quotes(const std::string &s) {
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
    return s.substr(1, s.size() - 2);
  return s;
}

static std::string read_file(const std::string &path, bool &ok) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    ok = false;
    return std::string();
  }
  ok = true;
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// Execute `curl` if available to fetch URL; this is a simple fallback when
// libcurl isn't used. Uses popen (works on Windows with msys/cygwin or
// on Windows 10/11 curl.exe present). Returns true on success.
static bool fetch_url_with_curl_cli(const std::string &url, std::string &out) {
  std::string cmd = "curl --fail -L -s " + url;
#if defined(_WIN32) || defined(_WIN64)
  FILE *pipe = _popen(cmd.c_str(), "r");
#else
  FILE *pipe = popen(cmd.c_str(), "r");
#endif
  if (!pipe)
    return false;
  char buffer[4096];
  out.clear();
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    out += buffer;
  }
#if defined(_WIN32) || defined(_WIN64)
  int rc = _pclose(pipe);
#else
  int rc = pclose(pipe);
#endif
  return (rc == 0);
}

// Portable detection of HTTP/HTTPS
static bool is_http_url(const std::string &s) {
  return (s.rfind("http://", 0) == 0) || (s.rfind("https://", 0) == 0);
}

bool init_schema_registry(const std::string &config_path, std::string &err) {
  bool ok;
  std::string content = read_file(config_path, ok);
  if (!ok) {
    err = "cannot read config file";
    return false;
  }
  // Very small parser: find "schemas": [ ... ] then for each object look for
  // "id" and "source"
  size_t pos = content.find("\"schemas\"");
  if (pos == std::string::npos) {
    err = "no schemas key";
    return false;
  }
  pos = content.find('[', pos);
  if (pos == std::string::npos) {
    err = "malformed schemas array";
    return false;
  }
  // Find matching closing ']' that balances nested arrays and ignores
  // brackets inside strings
  size_t end = std::string::npos;
  bool in_string = false;
  int depth = 0;
  for (size_t k = pos; k < content.size(); ++k) {
    char ch = content[k];
    if (ch == '"') {
      // toggle string state if not escaped
      bool escaped = (k > 0 && content[k - 1] == '\\');
      if (!escaped)
        in_string = !in_string;
    }
    if (in_string)
      continue;
    if (ch == '[')
      depth++;
    else if (ch == ']') {
      depth--;
      if (depth == 0) {
        end = k;
        break;
      }
    }
  }
  if (end == std::string::npos) {
    err = "malformed schemas array (no closing bracket)";
    return false;
  }
  if (end == std::string::npos) {
    err = "malformed schemas array (no closing bracket)";
    return false;
  }
  std::string arr = content.substr(pos + 1, end - pos - 1);
  // std::cerr << "[init_schema_registry] schemas array excerpt: '" << arr
  //           << "'\n";
  // arr segment debug will be printed per-object after obj_start is found
  // split by '},' to approximate object boundaries
  size_t cursor = 0;
  g_schema_registry.clear();
  while (cursor < arr.size()) {
    size_t obj_start = arr.find('{', cursor);
    if (obj_start == std::string::npos)
      break;
    size_t obj_end = arr.find('}', obj_start);
    // std::cerr << "[init_schema_registry] obj_start=" << obj_start
    //           << " obj_end=" << obj_end << " cursor=" << cursor
    //           << " arr.size=" << arr.size() << "\n";
    // std::cerr << "[init_schema_registry] arr segment='"
    //           << arr.substr(obj_start,
    //           std::min<size_t>(arr.size() - obj_start, 200))
    //           << "'\n";
    if (obj_end == std::string::npos)
      break;
    std::string obj = arr.substr(obj_start + 1, obj_end - obj_start - 1);
    // std::cerr << "[init_schema_registry] obj excerpt: '"
    //           << obj.substr(0, std::min<size_t>(obj.size(), 200)) << "'\n";
    cursor = obj_end + 1;
    SchemaEntry e;
    // find id
    size_t id_pos = obj.find("\"id\"");
    if (id_pos != std::string::npos) {
      size_t col = obj.find(':', id_pos);
      if (col != std::string::npos) {
        size_t quote = obj.find('"', col);
        size_t quote2 = obj.find('"', quote + 1);
        if (quote != std::string::npos && quote2 != std::string::npos)
          e.id = obj.substr(quote + 1, quote2 - quote - 1);
      }
    }
    size_t src_pos = obj.find("\"source\"");
    if (src_pos != std::string::npos) {
      size_t col = obj.find(':', src_pos);
      if (col != std::string::npos) {
        size_t quote = obj.find('"', col);
        size_t quote2 = obj.find('"', quote + 1);
        if (quote != std::string::npos && quote2 != std::string::npos)
          e.source = obj.substr(quote + 1, quote2 - quote - 1);
      }
    }
    // parse optional links array
    size_t links_pos = obj.find("\"links\"");
    if (links_pos != std::string::npos) {
      size_t lb = obj.find('[', links_pos);
      if (lb != std::string::npos) {
        size_t le = obj.find(']', lb);
        if (le != std::string::npos) {
          std::string links_arr = obj.substr(lb + 1, le - lb - 1);
          size_t cur2 = 0;
          while (cur2 < links_arr.size()) {
            size_t q1 = links_arr.find('"', cur2);
            if (q1 == std::string::npos)
              break;
            size_t q2 = links_arr.find('"', q1 + 1);
            if (q2 == std::string::npos)
              break;
            std::string link = links_arr.substr(q1 + 1, q2 - q1 - 1);
            if (!link.empty())
              e.links.push_back(link);
            cur2 = q2 + 1;
            size_t nxt = links_arr.find(',', cur2);
            if (nxt == std::string::npos)
              break;
            cur2 = nxt + 1;
          }
        }
      }
    }
    if (!e.id.empty() && !e.source.empty()) {
      g_schema_registry.push_back(e);
    }
  }
  // debug: how many schemas were parsed
  // std::cerr << "[init_schema_registry] loaded " << g_schema_registry.size()
  // << " entries\n";

  // Parse optional settings object: resolveRemote and cacheDirectory
  size_t settings_pos = content.find("\"settings\"");
  if (settings_pos != std::string::npos) {
    size_t sstart = content.find('{', settings_pos);
    if (sstart != std::string::npos) {
      size_t send = content.find('}', sstart);
      if (send != std::string::npos) {
        std::string settings = content.substr(sstart + 1, send - sstart - 1);
        size_t rr = settings.find("\"resolveRemote\"");
        if (rr != std::string::npos) {
          size_t col = settings.find(':', rr);
          if (col != std::string::npos) {
            size_t t = settings.find_first_not_of(" \t\n\r", col + 1);
            if (t != std::string::npos) {
              if (settings.compare(t, 4, "true") == 0)
                g_resolve_remote = true;
              else
                g_resolve_remote = false;
            }
          }
        }
        size_t cachep = settings.find("\"cacheDirectory\"");
        if (cachep != std::string::npos) {
          size_t col = settings.find(':', cachep);
          if (col != std::string::npos) {
            size_t quote = settings.find('"', col);
            size_t quote2 = settings.find('"', quote + 1);
            if (quote != std::string::npos && quote2 != std::string::npos)
              g_cache_dir = settings.substr(quote + 1, quote2 - quote - 1);
          }
        }
      }
    }
  }
  return true;
}

bool get_schema_source(const std::string &id_or_source, std::string &out,
                       std::string &err) {
  // If it's an id that exists in the registry, select its source
  std::string source = id_or_source;
  auto it = std::find_if(
      g_schema_registry.begin(), g_schema_registry.end(),
      [&](const SchemaEntry &se) { return se.id == id_or_source; });
  if (it != g_schema_registry.end()) {
    source = it->source;
  } else {
    // If it's an absolute or relative path to a file, try to read it
    bool ok;
    std::string local = read_file(id_or_source, ok);
    if (ok) {
      out = local;
      return true;
    }
    // if it's an http url, accept it, maybe idk
    if (is_http_url(id_or_source)) {
      source = id_or_source;
    } else {
      // report available ids in registry to make debugging easier for me :)
      std::string ids;
      for (const auto &s : g_schema_registry) {
        if (!ids.empty())
          ids += ", ";
        ids += s.id;
      }
      if (!ids.empty()) {
        err = "schema id '" + id_or_source +
              "' not found in registry; available ids: " + ids;
        return false;
      }
      err = "schema '" + id_or_source + "' not found";
      return false;
    }
  }

  if (is_http_url(source)) {
    if (!g_resolve_remote) {
      err = "remote fetching disabled by settings";
      return false;
    }
    // Simple cache mechanism: store by hashed filename in g_cache_dir if set
    std::string cache_file;
    if (!g_cache_dir.empty()) {
      // Try to create the directory if it does not exist.
#ifdef _WIN32
      if (!dir_exists(g_cache_dir))
        _mkdir(g_cache_dir.c_str());
#else
      if (!dir_exists(g_cache_dir))
        mkdir(g_cache_dir.c_str(), 0755);
#endif
      std::size_t h = std::hash<std::string>{}(source);
      cache_file = g_cache_dir + "/" + std::to_string(h) + ".json";
      bool ok;
      std::string cached = read_file(cache_file, ok);
      if (ok) {
        out = cached;
        return true;
      }
    }
#ifdef USE_CURL
    // TODO: add libcurl support : prob never cuz just.
#endif
    bool ok = fetch_url_with_curl_cli(source, out);
    if (!ok) {
      err = "failed to fetch url";
      return false;
    }
    // write to cache
    if (!cache_file.empty()) {
      std::ofstream fout(cache_file, std::ios::binary);
      if (fout)
        fout << out;
    }
    return true;
  }
  // Otherwise treat as local file
  bool ok;
  out = read_file(source, ok);
  if (!ok) {
    err = "cannot read file: " + source;
    return false;
  }
  return true;
}

std::vector<std::string> list_schema_ids() {
  std::vector<std::string> res;
  for (const auto &e : g_schema_registry)
    res.push_back(e.id);
  return res;
}

// Resolve schema and links recursively into out_map. Avoid cycles using a
// visited set.
static bool
resolve_schema_links_helper(const std::string &id_or_source,
                            std::map<std::string, std::string> &out_map,
                            std::set<std::string> &visited, std::string &err) {
  if (visited.count(id_or_source))
    return true;
  visited.insert(id_or_source);
  std::string content;
  if (!get_schema_source(id_or_source, content, err))
    return false;
  // prefer id key if available
  auto it =
      std::find_if(g_schema_registry.begin(), g_schema_registry.end(),
                   [&](const SchemaEntry &se) {
                     return se.source == id_or_source || se.id == id_or_source;
                   });
  std::string key = id_or_source;
  if (it != g_schema_registry.end())
    key = it->id;
  out_map[key] = content;

  // if the entry exists and has links, resolve them
  if (it != g_schema_registry.end()) {
    for (const auto &link : it->links) {
      if (!resolve_schema_links_helper(link, out_map, visited, err))
        return false;
    }
  }
  return true;
}

bool resolve_schema_links(const std::string &id_or_source,
                          std::map<std::string, std::string> &out_map,
                          std::string &err) {
  std::set<std::string> visited;
  return resolve_schema_links_helper(id_or_source, out_map, visited, err);
}