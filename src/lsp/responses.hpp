#pragma once

#include <variant>

#include "lsp/errors.hpp"
#include <nlohmann/json.hpp>

namespace lsp {

typedef nlohmann::ordered_json InitializeResult;

using Result = std::variant<InitializeResult>;

struct Response {
  int contentLength;
  nlohmann::ordered_json body;
};

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const Result &result) {
  if (std::holds_alternative<InitializeResult>(result)) {
    j = std::get<InitializeResult>(result);
  } else
    j = {};
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
