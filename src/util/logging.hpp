#pragma once

#include <iostream>
#include <optional>

#include "../lsp/errors.hpp"
#include <nlohmann/json.hpp>

inline void log_error(lsp::ErrorCode code,
                      const std::optional<nlohmann::json> &data = std::nullopt,
                      const std::string &message = "") {

  lsp::Error error(code, message.empty() ? getErrorMessage(code) : message,
                   data);

  std::cerr << "Error: " + error.message + " ";
  if (error.data.has_value())
    std::cerr << "Data: " << error.data.value() << std::endl;
  else
    std::cerr << std::endl;
}
