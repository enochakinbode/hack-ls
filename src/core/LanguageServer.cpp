#include "./LanguageServer.hpp"
#include "lsp/messages.hpp"
#include <nlohmann/json.hpp>

using namespace std;
using nlohmann::json;

void LanguageServer::start() {
  while (running) {

    auto body = io.read(); // read one full LSP message

    if (!body) {
      // This means EOF or invalid message
      cerr << "Failed to read message or connection closed.\n";
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
                                 std::string("Parse error: ") + e.what());
      continue;
    }

    messagesHandler.process(message);
  }
};
