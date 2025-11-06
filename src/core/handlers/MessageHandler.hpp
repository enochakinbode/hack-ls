#pragma once

#include <optional>
#include <variant>

#include "IServerInitState.hpp"
#include "core/handlers/DocumentHandler.hpp"
#include "core/transport/MessageIO.hpp"
#include "lsp/errors.hpp"
#include "lsp/messages.hpp"
#include "lsp/responses.hpp"
#include <nlohmann/json.hpp>

class MessageHandler {

public:
  MessageHandler(IServerIntailizationState &_server, IRespond &_io)
      : server(_server), io(_io) {};

  int process(nlohmann::json &_message);

private:
  IServerIntailizationState &server;
  IRespond &io;
  DocumentHandler documentHandler;

  int validateMessage(nlohmann::json &message);

  int processRequest(nlohmann::json &message);
  int handleNotification(nlohmann::json &message);

  lsp::InitializeResult initialize(lsp::RequestMessage &req);
  int didOpen(lsp::NotificationMessage &notif);
  int didChange(lsp::NotificationMessage &notif);

  void send_error_response(
      const std::variant<std::string, int> &id, lsp::ErrorCode errorCode,
      const char *message = nullptr,
      const std::optional<nlohmann::json> &data = std::nullopt) {

    std::string errorMessage =
        message ? std::string(message) : lsp::getErrorMessage(errorCode);
    lsp::Error error(errorCode, errorMessage, data);
    io.respond(id, std::variant<lsp::Result, lsp::Error>(error));
  }

  void send_response(const std::variant<std::string, int> &id,
                     const lsp::InitializeResult &result) {

    lsp::Result lspResult = result;
    io.respond(id, std::variant<lsp::Result, lsp::Error>(lspResult));
  }
};