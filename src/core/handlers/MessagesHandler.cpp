#include <optional>
#include <string>
#include <variant>

#include "MessagesHandler.hpp"
#include "lsp/params.hpp"
#include "lsp/protocol.hpp"
#include "lsp/responses.hpp"
#include "util/logging.hpp"

using namespace std;

int MessagesHandler::validateMessage(nlohmann::json &message) {
  try {
    message.at("jsonrpc");
    message.at("method");

    return 1;

  } catch (const std::exception &e) {
    if (message.contains("id")) {
      variant<string, int> id;
      auto &_id = message.at("id");

      _id.is_string() ? id = _id.get<string>() : id = _id.get<int>();
      send_error_response(id, lsp::ErrorCode::PARSE_ERROR, e.what());

      return 0;
    }

    log_error(lsp::ErrorCode::INVALID_MESSAGE, e.what());
    return 0;
  }
}

int MessagesHandler::process(nlohmann::json &message) {
  if (!validateMessage(message)) {
  }

  if (message.contains("id")) {
    return processRequest(message);
  } else
    return handleNotification(message);
}

int MessagesHandler::processRequest(nlohmann::json &message) {
  try {
    lsp::RequestMessage req(message);

    if (!server.isInitailized() && !(req.method == "initialize")) {
      send_error_response(req.id, lsp::ErrorCode::SERVER_NOT_INITIALIZED);
      return 1;
    }

    if (req.method == "initialize") {
      if (server.isInitailized()) {
        send_error_response(req.id, lsp::ErrorCode::REQUEST_CANCELLED,
                            "Server is already initialzed");
        return 1;
      }

      lsp::InitializeResult result = initialize(req);
      send_response(req.id, result);

      return 0;
    }

    send_error_response(req.id, lsp::ErrorCode::METHOD_NOT_FOUND);
    return 1;

  } catch (const std::exception &e) {
    variant<string, int> id;
    auto &_id = message.at("id");

    _id.is_string() ? id = _id.get<string>() : id = _id.get<int>();
    send_error_response(id, lsp::ErrorCode::PARSE_ERROR, e.what());

    return 1;
  }
}

int MessagesHandler::handleNotification(nlohmann::json &message) {
  if (!server.isInitailized()) {
    log_error(lsp::ErrorCode::SERVER_NOT_INITIALIZED);
    return 1;
  }

  try {
    lsp::NotificationMessage notif(message);

    if (notif.method == "textDocument/didOpen")
      return didOpen(notif);

    if (notif.method == "textDocument/didChange")
      return didChange(notif);

    log_error(lsp::ErrorCode::METHOD_NOT_FOUND);
    return 1;

  } catch (const lsp::Error &e) {
    log_error(e.code, e.data, e.message);
    return 1;

  } catch (const std::exception &e) {
    log_error(lsp::ErrorCode::INVALID_PARAMS, e.what());
    return 1;
  }
}

lsp::InitializeResult MessagesHandler::initialize(lsp::RequestMessage &req) {

  auto _params = req.params;
  lsp::InitializeParams params(_params);

  lsp::InitializeResult result = protocol::serverDetails::to_json();

  server.onInitialize();
  return result;
}

int MessagesHandler::didOpen(lsp::NotificationMessage &notif) {

  auto _params = notif.params.value();
  lsp::DidOpenParams didOpenParams(_params);
  documentsHandler.onOpen(didOpenParams);

  hackManager.processDocument(didOpenParams.textDocument.uri);
  return 0;
}

int MessagesHandler::didChange(lsp::NotificationMessage &notif) {

  auto _params = notif.params.value();
  lsp::DidChangeParams didChangeParams(_params);
  documentsHandler.onChange(didChangeParams);

  hackManager.processDocument(didChangeParams.textDocument.uri);
  return 0;
}
