#pragma once

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <variant>

#include "core/interfaces/IRespond.hpp"
#include "lsp/errors.hpp"
#include "lsp/responses.hpp"
#include <nlohmann/json.hpp>

class MessageIO : public IRespond {
public:
  std::optional<std::string> read() noexcept {
    std::map<std::string, std::string> headers;
    std::string line;

    // Read headers
    while (std::getline(std::cin, line)) {
      if (!line.empty() && line.back() == '\r')
        line.pop_back(); // Remove CR if CRLF

      if (line.empty())
        break; // End of headers

      parse_header(headers, line); // We ignore invalid headers gracefully
    }

    // If EOF before finishing headers
    if (!std::cin && line != "")
      return std::nullopt;

    // Get Content-Length
    int contentLength = 0;
    if (headers.count("content-length")) {
      try {
        contentLength = std::stoi(headers["content-length"]);
      } catch (...) {
        return std::nullopt; // Invalid number format
      }
    }

    if (contentLength <= 0)
      return std::string(); // No body

    // Read body
    std::string content(contentLength, '\0');
    std::cin.read(content.data(), contentLength);
    if (std::cin.gcount() < contentLength) {
      return std::nullopt; // Incomplete body
    }

    return content;
  }

  void respond(
      const nlohmann::json &id,
      const std::variant<lsp::Result, lsp::Error> &response) noexcept override {

    auto res = generate_response(id, response);

    {
      std::lock_guard<std::mutex> lock(writeMutex);
      std::cout << "Content-Length: " << res.contentLength << "\r\n\r\n";
      std::cout << res.body;
      std::cout.flush();
    }
  }

  void sendNotification(const std::string &method,
                        const nlohmann::json &params) override {

    nlohmann::ordered_json message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;

    if (!params.is_null()) {
      // For window/logMessage, ensure type comes before message
      if (method == "window/logMessage" && params.is_object() &&
          params.contains("type") && params.contains("message")) {
        nlohmann::ordered_json orderedParams;
        orderedParams["type"] = params["type"];
        orderedParams["message"] = params["message"];
        message["params"] = orderedParams;
      } else {
        // Convert to ordered_json to preserve key order for other notifications
        message["params"] = nlohmann::ordered_json(params);
      }
    }

    const std::string body = message.dump();
    {
      std::lock_guard<std::mutex> lock(writeMutex);
      std::cout << "Content-Length: " << body.size() << "\r\n\r\n";
      std::cout << body;
      std::cout.flush();
    }
  }

private:
  bool parse_header(std::map<std::string, std::string> &headers,
                    const std::string &line) {

    auto pos = line.find(':');
    if (pos == std::string::npos) {
      return false; // Invalid header (no ':')
    }

    // Extract key and value
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    // Trim spaces (\t or space) from both sides
    auto trim = [](std::string &s) {
      size_t start = s.find_first_not_of(" \t");
      if (start == std::string::npos) {
        s.clear();
        return;
      }
      size_t end = s.find_last_not_of(" \t");
      s = s.substr(start, end - start + 1);
    };

    trim(key);
    trim(value);

    // Normalize header key to lowercase (optional but common for LSP)
    for (auto &c : key)
      c = std::tolower(c);

    headers[key] = value;
    return true;
  }

  lsp::Response
  generate_response(const nlohmann::json &id,
                    const std::variant<lsp::Result, lsp::Error> &result) {

    nlohmann::ordered_json msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = id;

    if (std::holds_alternative<lsp::Result>(result)) {
      nlohmann::ordered_json resultJson;
      lsp::to_json(resultJson, std::get<lsp::Result>(result));
      msg["result"] = resultJson;
    } else {
      nlohmann::ordered_json errorJson;
      lsp::to_json(errorJson, std::get<lsp::Error>(result));
      msg["error"] = errorJson;
    }

    int contentlength = static_cast<int>(msg.dump().size());
    return {contentlength, msg};
  }

  std::mutex writeMutex;
};
