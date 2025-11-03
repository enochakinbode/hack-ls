#ifndef LSP_RESPONSES_HPP
#define LSP_RESPONSES_HPP

#include <exception>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>

namespace lsp {

struct ServerInfo {
  std::string name;
  std::string version;
};

struct ServerCapabilities {
  std::string positionEncoding;
  bool hoverProvider;
  bool definitionProvider;
};

struct InitializeResult {
  ServerInfo serverInfo;
  ServerCapabilities serverCapabilities;
};

using Result = std::variant<InitializeResult>;

enum class ErrorCode : int {
  // JSON-RPC 2.0 standard error codes
  PARSE_ERROR = -32700,
  INVALID_MESSAGE = -32600,
  INTERNAL_ERROR = -32603,

  // LSP specific error codes
  METHOD_NOT_FOUND = -32601,
  INVALID_PARAMS = -32602,
  SERVER_ERROR_START = -32099,
  SERVER_ERROR_END = -32000,
  SERVER_NOT_INITIALIZED = -32002,
  UNKNOWN_ERROR_CODE = -32001,
  REQUEST_CANCELLED = -32800,
  CONTENT_MODIFIED = -32801
};

// Returns the default message for a given ErrorCode
inline std::string getErrorMessage(ErrorCode code) {
  switch (code) {
  case ErrorCode::PARSE_ERROR:
    return "Parse error";
  case ErrorCode::INVALID_MESSAGE:
    return "Invalid Request";
  case ErrorCode::METHOD_NOT_FOUND:
    return "Method not found";
  case ErrorCode::INVALID_PARAMS:
    return "Invalid params";
  case ErrorCode::INTERNAL_ERROR:
    return "Internal error";
  case ErrorCode::SERVER_NOT_INITIALIZED:
    return "Server not initialized";
  case ErrorCode::UNKNOWN_ERROR_CODE:
    return "Unknown error code";
  case ErrorCode::REQUEST_CANCELLED:
    return "Request cancelled";
  case ErrorCode::CONTENT_MODIFIED:
    return "Content modified";
  default:
    return "Unknown error";
  }
}

class Error : public std::exception {

public:
  ErrorCode code;
  std::string message;
  std::optional<nlohmann::json> data;

  Error(lsp::ErrorCode code, std::string msg,
        std::optional<nlohmann::json> data)
      : code(code), message(std::move(msg)), data(data) {}

  const char *what() const noexcept override { return message.c_str(); }
};

struct LSPErrorResponse {
  int contentLength;
  nlohmann::ordered_json error;
};

struct LSPResponse {
  int contentLength;
  nlohmann::ordered_json result;
};

// JSON conversion functions - work with both json and ordered_json

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const InitializeResult &result) {
  ServerCapabilities caps = result.serverCapabilities;
  ServerInfo serverInfo = result.serverInfo;

  j["serverCapabilities"]["hoverProvider"] = caps.hoverProvider;
  j["serverCapabilities"]["definitionProvider"] = caps.definitionProvider;
  j["serverCapabilities"]["positionEncoding"] = caps.positionEncoding;

  j["serverInfo"]["name"] = serverInfo.name;
  j["serverInfo"]["version"] = serverInfo.version;
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const Result &result) {
  std::visit([&j](const auto &res) { j = res; }, result);
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const Error &error) {
  j["code"] = static_cast<int>(error.code);
  j["message"] = error.message;
  if (error.data.has_value()) {
    j["data"] = error.data.value();
  }
}

} // namespace lsp

#endif // LSP_RESPONSES_HPP
