#pragma once

#include <exception>
#include <string>

#include <nlohmann/json.hpp>

namespace lsp {

enum class ErrorCode : int {
  // JSON-RPC 2.0 standard error codes
  PARSE_ERROR = -32700,
  INVALID_MESSAGE = -32600,
  INTERNAL_ERROR = -32603,

  // LSP specific error codes
  METHOD_NOT_FOUND = -32601,
  INVALID_PARAMS = -32602,
  SERVER_NOT_INITIALIZED = -32002
};

class Error : public std::exception {

public:
  ErrorCode code;
  std::optional<nlohmann::json> data;

  Error(lsp::ErrorCode code, std::string msg,
        std::optional<nlohmann::json> data)
      : code(code), data(data), message(std::move(msg)) {}

  const char *what() const noexcept override { return message.c_str(); }

private:
  std::string message;
};

// Returns the default message for a given ErrorCode
inline const char *getErrorMessage(lsp::ErrorCode code) {
  switch (code) {
  case lsp::ErrorCode::PARSE_ERROR:
    return "Parse error";
  case lsp::ErrorCode::INVALID_MESSAGE:
    return "Invalid Request";
  case lsp::ErrorCode::METHOD_NOT_FOUND:
    return "Method not found";
  case lsp::ErrorCode::INVALID_PARAMS:
    return "Invalid params";
  case lsp::ErrorCode::INTERNAL_ERROR:
    return "Internal error";
  case lsp::ErrorCode::SERVER_NOT_INITIALIZED:
    return "Server not initialized";
  default:
    return "Unknown error";
  }
}

} // namespace lsp