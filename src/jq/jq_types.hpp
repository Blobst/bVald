#ifndef JQ_TYPES_HPP
#define JQ_TYPES_HPP

#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Forward declarations
struct JsonValue;

namespace jls {
struct Value;
}

namespace jq {

// ================= jq Value Type =================
// Unified value representation that can convert to/from libjsonval and JLS.
// Uses shared_ptr for RAII memory management.

enum class ValueType {
  JV_NULL,
  JV_BOOLEAN,
  JV_NUMBER,
  JV_STRING,
  JV_ARRAY,
  JV_OBJECT
};

class JvValue;
using JvValuePtr = std::shared_ptr<JvValue>;

class JvValue {
public:
  ValueType type;
  bool b = false;                      // boolean
  double n = 0.0;                      // number
  std::string s;                       // string
  std::vector<JvValuePtr> a;           // array
  std::map<std::string, JvValuePtr> o; // object

  // Constructors
  JvValue() : type(ValueType::JV_NULL) {}
  explicit JvValue(bool v) : type(ValueType::JV_BOOLEAN), b(v) {}
  explicit JvValue(double v) : type(ValueType::JV_NUMBER), n(v) {}
  explicit JvValue(int v) : type(ValueType::JV_NUMBER), n(v) {}
  explicit JvValue(long long v) : type(ValueType::JV_NUMBER), n(v) {}
  explicit JvValue(const std::string &v) : type(ValueType::JV_STRING), s(v) {}
  explicit JvValue(const char *v) : type(ValueType::JV_STRING), s(v) {}

  // Static constructors
  static JvValuePtr null() { return std::make_shared<JvValue>(); }

  static JvValuePtr boolean(bool v) { return std::make_shared<JvValue>(v); }

  static JvValuePtr number(double v) { return std::make_shared<JvValue>(v); }

  static JvValuePtr string(const std::string &v) {
    return std::make_shared<JvValue>(v);
  }

  static JvValuePtr array() {
    auto val = std::make_shared<JvValue>();
    val->type = ValueType::JV_ARRAY;
    return val;
  }

  static JvValuePtr object() {
    auto val = std::make_shared<JvValue>();
    val->type = ValueType::JV_OBJECT;
    return val;
  }

  // Type checks
  bool is_null() const { return type == ValueType::JV_NULL; }
  bool is_bool() const { return type == ValueType::JV_BOOLEAN; }
  bool is_number() const { return type == ValueType::JV_NUMBER; }
  bool is_string() const { return type == ValueType::JV_STRING; }
  bool is_array() const { return type == ValueType::JV_ARRAY; }
  bool is_object() const { return type == ValueType::JV_OBJECT; }

  // Utility
  bool is_integer() const {
    return type == ValueType::JV_NUMBER && n == std::floor(n);
  }

  long long as_integer() const { return static_cast<long long>(n); }

  // Array/Object access
  JvValuePtr array_index(size_t i) const {
    if (type != ValueType::JV_ARRAY || i >= a.size())
      return null();
    return a[i];
  }

  JvValuePtr object_get(const std::string &key) const {
    if (type != ValueType::JV_OBJECT) {
      return null();
    }
    auto it = o.find(key);
    if (it == o.end())
      return null();
    return it->second;
  }

  void array_push(const JvValuePtr &v) {
    if (type == ValueType::JV_ARRAY) {
      a.push_back(v);
    }
  }

  void object_set(const std::string &key, const JvValuePtr &v) {
    if (type == ValueType::JV_OBJECT) {
      o[key] = v;
    }
  }

  // Conversion to/from string representation (for debugging)
  std::string to_string() const;
  static JvValuePtr from_string(const std::string &json_text, std::string &err);
};

// ================= Converters =================

// Convert from libjsonval JsonValue to jq JvValue
JvValuePtr from_json_value(const JsonValue &jv);

// Convert from jq JvValue to libjsonval JsonValue
JsonValue to_json_value(const JvValuePtr &jv);

// Convert from JLS Value to jq JvValue
JvValuePtr from_jls_value(const std::shared_ptr<jls::Value> &v);

// Convert from jq JvValue to JLS Value
std::shared_ptr<jls::Value> to_jls_value(const JvValuePtr &jv);

// ================= Error Handling =================

class JqError : public std::exception {
private:
  std::string message;

public:
  explicit JqError(const std::string &msg) : message(msg) {}
  const char *what() const noexcept override { return message.c_str(); }
};

} // namespace jq

#endif // JQ_TYPES_HPP
