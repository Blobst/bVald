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

const std::string VERSION = "0.1.4";

struct JsonParser {
  const std::string &s;
  size_t i = 0;
  std::string err;

  JsonParser(const std::string &str) : s(str), i(0) {}

  void skip_ws() {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
      ++i;
  }

  bool parse_value() {
    skip_ws();
    if (i >= s.size()) {
      err = "unexpected end of input";
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
    err = std::string("unexpected character '") + c + "'";
    return false;
  }

  bool parse_object() {
    if (s[i] != '{') {
      err = "expected '{'";
      return false;
    }
    ++i;
    skip_ws();
    if (i < s.size() && s[i] == '}') {
      ++i;
      return true;
    }
    while (true) {
      skip_ws();
      if (!parse_string())
        return false;
      skip_ws();
      if (i >= s.size() || s[i] != ':') {
        err = "expected ':' after object key";
        return false;
      }
      ++i;
      if (!parse_value())
        return false;
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == '}') {
        ++i;
        return true;
      }
      err = "expected ',' or '}' in object";
      return false;
    }
  }

  bool parse_array() {
    if (s[i] != '[') {
      err = "expected '['";
      return false;
    }
    ++i;
    skip_ws();
    if (i < s.size() && s[i] == ']') {
      ++i;
      return true;
    }
    while (true) {
      if (!parse_value())
        return false;
      skip_ws();
      if (i < s.size() && s[i] == ',') {
        ++i;
        continue;
      }
      if (i < s.size() && s[i] == ']') {
        ++i;
        return true;
      }
      err = "expected ',' or ']' in array";
      return false;
    }
  }

  bool parse_string() {
    if (s[i] != '"') {
      err = "expected '\"'";
      return false;
    }
    ++i;
    while (i < s.size()) {
      char c = s[i++];
      if (c == '"')
        return true;
      if (c == '\\') {
        if (i >= s.size()) {
          err = "unterminated escape in string";
          return false;
        }
        char e = s[i++];
        if (e == 'u') {
          for (int k = 0; k < 4; ++k) {
            if (i >= s.size() || !is_hex(s[i++])) {
              err = "invalid unicode escape in string";
              return false;
            }
          }
        } else {
          if (e != '"' && e != '\\' && e != '/' && e != 'b' && e != 'f' &&
              e != 'n' && e != 'r' && e != 't') {
            err = std::string("invalid escape: ") + e;
            return false;
          }
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        err = "control character in string";
        return false;
      }
    }
    err = "unterminated string";
    return false;
  }

  static bool is_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
  }

  bool parse_number() {
    size_t start = i;
    if (s[i] == '-')
      ++i;
    if (i >= s.size()) {
      err = "invalid number";
      return false;
    }
    if (s[i] == '0') {
      ++i;
    } else if (s[i] >= '1' && s[i] <= '9') {
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
        ++i;
    } else {
      err = "invalid number";
      return false;
    }
    if (i < s.size() && s[i] == '.') {
      ++i;
      if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) {
        err = "invalid fractional part in number";
        return false;
      }
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
        ++i;
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
      ++i;
      if (i < s.size() && (s[i] == '+' || s[i] == '-'))
        ++i;
      if (i >= s.size() || !isdigit(static_cast<unsigned char>(s[i]))) {
        err = "invalid exponent in number";
        return false;
      }
      while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
        ++i;
    }
    return i > start;
  }

  bool parse_literal() {
    if (s.compare(i, 4, "true") == 0) {
      i += 4;
      return true;
    }
    if (s.compare(i, 5, "false") == 0) {
      i += 5;
      return true;
    }
    if (s.compare(i, 4, "null") == 0) {
      i += 4;
      return true;
    }
    err = "invalid literal";
    return false;
  }
};

bool validate_json(const std::string &text, std::string &error_msg) {
  JsonParser p(text);
  p.skip_ws();
  if (!p.parse_value()) {
    error_msg = p.err;
    return false;
  }
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
               "print info\n";
}

// Simple schema registry implementation (parsing `schemas.json` in a robust way
// is out of scope; we accept a lightweight parser that extracts `schemas[].id`
// and `schemas[].source` fields.)

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
  // Find matching closing ']' that balances nested arrays and ignores brackets
  // inside strings
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
  if (it != g_schema_registry.end())
    source = it->source;
  else {
    // report available ids in registry to make debugging easier
    std::string ids;
    for (const auto &s : g_schema_registry) {
      if (!ids.empty())
        ids += ", ";
      ids += s.id;
    }
    if (!ids.empty()) {
      if (!is_http_url(id_or_source)) {
        err = "schema id '" + id_or_source +
              "' not found in registry; available ids: " + ids;
        return false;
      }
      // otherwise argument is a URL string; attempt to fetch it directly
      source = id_or_source;
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
    // TODO: add libcurl support
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