#include "./LanguageServer.hpp"
#include "lsp/errors.hpp"
#include "lsp/messages.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using nlohmann::json;

void LanguageServer::start() {
  while (running) {

    auto body = io.readMessage(); // read one full LSP message

    if (!body) {
      // This means EOF or invalid message
      std::cerr << "Failed to read message or connection closed.\n";
      break;
    }

    if (body->empty()) {
      // No content? Likely a notification with no body OR just keep-alive
      continue;
    }

    json message;
    try {
      message = json::parse(body.value());

    } catch (const std::exception &e) {
      messagesHandler.logMessage(MessageType::Error,
                                 lsp::ErrorCode::PARSE_ERROR, e.what());
      continue;
    }

    messagesHandler.process(message);

    // Check if we should exit after processing the message
    if (shouldExit()) {
      break;
    }
  }
}
