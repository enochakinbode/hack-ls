#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

enum class MessageType : int { Error = 1, Warning = 2, Info = 3, Log = 4 };

namespace lsp {

struct Message {
  std::string jsonrpc;
};

struct RequestMessage : public Message {
  nlohmann::json id;
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
  j.at("id").get_to(req.id);
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
