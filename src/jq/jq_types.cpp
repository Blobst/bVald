#include "jq_types.hpp"
#include "../../include/jls.hpp"
#include "../../include/libjsonval.hpp"
#include <sstream>

namespace jq {

// ================= JvValue to String =================

std::string JvValue::to_string() const {
  switch (type) {
  case ValueType::JV_NULL:
    return "null";
  case ValueType::JV_BOOLEAN:
    return b ? "true" : "false";
  case ValueType::JV_NUMBER: {
    if (is_integer()) {
      return std::to_string(as_integer());
    } else {
      std::ostringstream oss;
      oss << n;
      return oss.str();
    }
  }
  case ValueType::JV_STRING: {
    // Simple JSON string escaping
    std::string escaped;
    escaped += '"';
    for (char c : s) {
      switch (c) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
      }
    }
    escaped += '"';
    return escaped;
  }
  case ValueType::JV_ARRAY: {
    std::string result = "[";
    for (size_t i = 0; i < a.size(); ++i) {
      if (i > 0)
        result += ",";
      result += a[i]->to_string();
    }
    result += "]";
    return result;
  }
  case ValueType::JV_OBJECT: {
    std::string result = "{";
    bool first = true;
    for (const auto &kv : o) {
      if (!first)
        result += ",";
      result += "\"" + kv.first + "\":" + kv.second->to_string();
      first = false;
    }
    result += "}";
    return result;
  }
  }
  return "null";
}

// ================= String to JvValue =================

JvValuePtr JvValue::from_string(const std::string &json_text,
                                std::string &err) {
  JsonValue dom;
  if (!parse_json_dom(json_text, dom, err)) {
    return null();
  }
  return from_json_value(dom);
}

// ================= Converters =================

JvValuePtr from_json_value(const JsonValue &jv) {
  auto result = std::make_shared<JvValue>();

  switch (jv.t) {
  case JsonValue::T_NULL:
    result->type = ValueType::JV_NULL;
    break;
  case JsonValue::T_BOOL:
    result->type = ValueType::JV_BOOLEAN;
    result->b = jv.b;
    break;
  case JsonValue::T_NUMBER:
    result->type = ValueType::JV_NUMBER;
    result->n = jv.n;
    break;
  case JsonValue::T_STRING:
    result->type = ValueType::JV_STRING;
    result->s = jv.s;
    break;
  case JsonValue::T_ARRAY:
    result->type = ValueType::JV_ARRAY;
    for (const auto &elem : jv.a) {
      result->a.push_back(from_json_value(elem));
    }
    break;
  case JsonValue::T_OBJECT:
    result->type = ValueType::JV_OBJECT;
    for (const auto &kv : jv.o) {
      result->o[kv.first] = from_json_value(kv.second);
    }
    break;
  }

  return result;
}

JsonValue to_json_value(const JvValuePtr &jv) {
  JsonValue result;

  if (!jv) {
    result.t = JsonValue::T_NULL;
    return result;
  }

  switch (jv->type) {
  case ValueType::JV_NULL:
    result.t = JsonValue::T_NULL;
    break;
  case ValueType::JV_BOOLEAN:
    result.t = JsonValue::T_BOOL;
    result.b = jv->b;
    break;
  case ValueType::JV_NUMBER:
    result.t = JsonValue::T_NUMBER;
    result.n = jv->n;
    break;
  case ValueType::JV_STRING:
    result.t = JsonValue::T_STRING;
    result.s = jv->s;
    break;
  case ValueType::JV_ARRAY:
    result.t = JsonValue::T_ARRAY;
    for (const auto &elem : jv->a) {
      result.a.push_back(to_json_value(elem));
    }
    break;
  case ValueType::JV_OBJECT:
    result.t = JsonValue::T_OBJECT;
    for (const auto &kv : jv->o) {
      result.o[kv.first] = to_json_value(kv.second);
    }
    break;
  }

  return result;
}

JvValuePtr from_jls_value(const std::shared_ptr<jls::Value> &v) {
  if (!v) {
    return JvValue::null();
  }

  auto result = std::make_shared<JvValue>();

  using JlsValueType = jls::ValueType;
  switch (v->type) {
  case JlsValueType::NIL:
    result->type = ValueType::JV_NULL;
    break;
  case JlsValueType::BOOLEAN:
    result->type = ValueType::JV_BOOLEAN;
    result->b = v->b;
    break;
  case JlsValueType::INTEGER:
    result->type = ValueType::JV_NUMBER;
    result->n = v->i;
    break;
  case JlsValueType::FLOAT:
    result->type = ValueType::JV_NUMBER;
    result->n = v->f;
    break;
  case JlsValueType::STRING:
    result->type = ValueType::JV_STRING;
    result->s = v->s;
    break;
  case JlsValueType::LIST:
    result->type = ValueType::JV_ARRAY;
    for (const auto &item : v->list) {
      result->a.push_back(from_jls_value(item));
    }
    break;
  case JlsValueType::MAP:
    result->type = ValueType::JV_OBJECT;
    for (const auto &kv : v->map) {
      result->o[kv.first] = from_jls_value(kv.second);
    }
    break;
  default:
    // FUNCTION, LAMBDA - cannot convert
    result->type = ValueType::JV_NULL;
    break;
  }

  return result;
}

std::shared_ptr<jls::Value> to_jls_value(const JvValuePtr &jv) {
  if (!jv) {
    return std::make_shared<jls::Value>();
  }

  auto result = std::make_shared<jls::Value>();

  switch (jv->type) {
  case ValueType::JV_NULL:
    result->type = jls::ValueType::NIL;
    break;
  case ValueType::JV_BOOLEAN:
    result->type = jls::ValueType::BOOLEAN;
    result->b = jv->b;
    break;
  case ValueType::JV_NUMBER:
    if (jv->is_integer()) {
      result->type = jls::ValueType::INTEGER;
      result->i = jv->as_integer();
    } else {
      result->type = jls::ValueType::FLOAT;
      result->f = jv->n;
    }
    break;
  case ValueType::JV_STRING:
    result->type = jls::ValueType::STRING;
    result->s = jv->s;
    break;
  case ValueType::JV_ARRAY:
    result->type = jls::ValueType::LIST;
    for (const auto &elem : jv->a) {
      result->list.push_back(to_jls_value(elem));
    }
    break;
  case ValueType::JV_OBJECT:
    result->type = jls::ValueType::MAP;
    for (const auto &kv : jv->o) {
      result->map[kv.first] = to_jls_value(kv.second);
    }
    break;
  }

  return result;
}

} // namespace jq
