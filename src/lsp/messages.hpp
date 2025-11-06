#pragma once

#include <optional>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

namespace lsp {

struct Message {
  std::string jsonrpc;
};

struct RequestMessage : public Message {
  std::variant<std::string, int> id;
  std::string method;
  nlohmann::json params;
};

struct NotificationMessage : public Message {
  std::string method;
  std::optional<nlohmann::json> params;
};

// JSON conversion functions
inline void from_json(const nlohmann::json &j, RequestMessage &req) {
  j.at("jsonrpc").get_to(req.jsonrpc);
  if (j.at("id").is_string()) {
    req.id = j.at("id").get<std::string>();
  } else {
    req.id = j.at("id").get<int>();
  }
  j.at("method").get_to(req.method);
  if (j.contains("params")) {
    req.params = j.at("params");
  }
}

inline void from_json(const nlohmann::json &j, NotificationMessage &req) {
  j.at("jsonrpc").get_to(req.jsonrpc);
  j.at("method").get_to(req.method);
  if (j.contains("params")) {
    req.params = j.at("params");
  }
}

} // namespace lsp
