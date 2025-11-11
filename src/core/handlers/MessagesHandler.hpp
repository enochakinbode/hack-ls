#pragma once

#include <optional>

#include "core/handlers/DocumentsHandler.hpp"
#include "core/interfaces/IMessage.hpp"
#include "core/interfaces/IServerState.hpp"
#include "hack/HackManager.hpp"
#include "lsp/errors.hpp"
#include "lsp/messages.hpp"
#include "lsp/responses.hpp"
#include <nlohmann/json.hpp>

class MessagesHandler {

public:
  MessagesHandler(IServerState &_server, IMessage &_io)
      : server(_server), io(_io), hackManager(documentsHandler, _io) {};

  ~MessagesHandler() {
    // Free all assembler results on shutdown to prevent memory leaks
    hackManager.freeAllResults();
  }

  int process(nlohmann::json &_message);

  // Send a logMessage notification
  void logMessage(MessageType type, const std::string &message);
  // Log an error message from ErrorCode
  void logError(MessageType type, lsp::ErrorCode code,
                const char *additionalInfo = nullptr);

private:
  IServerState &server;
  IMessage &io;
  DocumentsHandler documentsHandler;
  HackManager hackManager;

  int processRequest(nlohmann::json &message);
  int handleNotification(nlohmann::json &message);

  // requests
  lsp::InitializeResult initialize(lsp::RequestMessage &req);
  lsp::CompletionResult completion(lsp::RequestMessage &req);
  lsp::HoverResult hover(lsp::RequestMessage &req);

  // notifications
  int initialized();
  int didOpen(lsp::NotificationMessage &notif);
  int didChange(lsp::NotificationMessage &notif);
  int didClose(lsp::NotificationMessage &notif);

  int validateMessage(nlohmann::json &message) {
    try {
      message.at("jsonrpc");
      message.at("method");
      return 1;

    } catch (const std::exception &e) {

      if (message.contains("id")) {
        send_response(message.at("id"), lsp::ErrorCode::PARSE_ERROR, e.what());
        return 0;
      }

      logError(MessageType::Error, lsp::ErrorCode::INVALID_MESSAGE, e.what());
      return 0;
    }
  }

  void send_response(const nlohmann::json &id,
                     const lsp::Result &result) noexcept {
    io.sendMessage(id, std::variant<lsp::Result, lsp::Error>(result));
  }

  void send_response(const nlohmann::json &id, lsp::ErrorCode code,
                     const char *message = nullptr,
                     const std::optional<nlohmann::json> &data = std::nullopt) {

    std::string errorMessage =
        message ? std::string(message) : lsp::getErrorMessage(code);
    lsp::Error error(code, errorMessage, data);
    io.sendMessage(id, std::variant<lsp::Result, lsp::Error>(error));
  }
};