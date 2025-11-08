#include <optional>
#include <string>

#include "MessagesHandler.hpp"
#include "lsp/params.hpp"
#include "lsp/protocol.hpp"
#include "lsp/responses.hpp"

int MessagesHandler::process(nlohmann::json &message) {

  if (!validateMessage(message))
    return 0;

  if (message.contains("id")) {
    return processRequest(message);
  } else
    return handleNotification(message);
}

int MessagesHandler::processRequest(nlohmann::json &message) {
  try {
    lsp::RequestMessage req(message);

    if (!server.isInitialized() && !(req.method == "initialize")) {
      send_response(req.id, lsp::ErrorCode::SERVER_NOT_INITIALIZED);
      return 1;
    }

    // After shutdown, only accept exit notification (handled in
    // handleNotification) Reject all other requests
    if (server.isShutdownRequested()) {
      send_response(req.id, lsp::ErrorCode::INVALID_MESSAGE,
                    "Server is shutting down");
      return 1;
    }

    if (req.method == "initialize") {
      lsp::InitializeResult result = initialize(req);
      send_response(req.id, lsp::Result(result));
      return 0;
    }

    if (req.method == "textDocument/completion") {
      lsp::CompletionResult result = completion(req);
      send_response(req.id, lsp::Result(result));
      return 0;
    }

    if (req.method == "shutdown") {
      server.onShutdown();
      send_response(req.id, lsp::Result(nullptr));
      return 0;
    }

    send_response(req.id, lsp::ErrorCode::METHOD_NOT_FOUND);
    return 1;

  } catch (const lsp::Error &e) {
    send_response(message.at("id"), e.code, e.what(), e.data);
    return 1;

  } catch (const std::exception &e) {

    nlohmann::json id = message.at("id");
    send_response(id, lsp::ErrorCode::PARSE_ERROR, e.what());

    return 1;
  }
}

int MessagesHandler::handleNotification(nlohmann::json &message) {
  try {
    lsp::NotificationMessage notif(message);

    // exit notification can be sent even if not initialized
    if (notif.method == "exit") {
      server.onExit();
      return 0;
    }

    if (!server.isInitialized()) {
      logMessage(MessageType::Error, lsp::ErrorCode::SERVER_NOT_INITIALIZED);
      return 1;
    }

    if (notif.method == "initialized")
      return initialized(notif);

    if (notif.method == "textDocument/didOpen")
      return didOpen(notif);

    if (notif.method == "textDocument/didChange")
      return didChange(notif);

    if (notif.method == "textDocument/didClose")
      return didClose(notif);

    logMessage(MessageType::Error, lsp::ErrorCode::METHOD_NOT_FOUND,
               notif.method.c_str());
    return 1;

  } catch (const lsp::Error &e) {
    std::string errorMsg = e.what();
    if (e.data.has_value()) {
      errorMsg += " (data: " + e.data.value().dump() + ")";
    }
    logMessage(MessageType::Error, errorMsg);
    return 1;

  } catch (const std::exception &e) {
    logMessage(MessageType::Error, lsp::ErrorCode::INVALID_PARAMS, e.what());
    return 1;
  }
}

lsp::InitializeResult MessagesHandler::initialize(lsp::RequestMessage &) {

  lsp::InitializeResult result = protocol::serverDetails::to_json();
  server.onInitialize();
  return result;
}

lsp::CompletionResult MessagesHandler::completion(lsp::RequestMessage &req) {

  lsp::CompletionParams params(req.params);

  auto &documents = documentsHandler.getDocuments();
  auto it = documents.find(params.textDocument.uri);

  if (it == documents.end()) {
    lsp::Error error(lsp::ErrorCode::INTERNAL_ERROR, "URI not found",
                     std::nullopt);
    throw error;
  }

  return hackManager.completion(params);
}

int MessagesHandler::initialized(lsp::NotificationMessage &) { return 0; }

int MessagesHandler::didOpen(lsp::NotificationMessage &notif) {

  auto _params = notif.params.value();
  lsp::DidOpenParams didOpenParams(_params);
  documentsHandler.onOpen(didOpenParams);

  std::string uri = didOpenParams.textDocument.uri;
  submitTask([this, uri]() { hackManager.processDocument(uri); });
  return 0;
}

int MessagesHandler::didChange(lsp::NotificationMessage &notif) {

  auto _params = notif.params.value();
  lsp::DidChangeParams didChangeParams(_params);
  documentsHandler.onChange(didChangeParams);

  std::string uri = didChangeParams.textDocument.uri;
  submitTask([this, uri]() { hackManager.processDocument(uri); });
  return 0;
}

int MessagesHandler::didClose(lsp::NotificationMessage &notif) {
  auto _params = notif.params.value();
  lsp::DidCloseParams didCloseParams(_params);
  std::string uri = didCloseParams.textDocument.uri;

  documentsHandler.onClose(didCloseParams);
  hackManager.freeURIResult(uri);

  return 0;
}

void MessagesHandler::submitTask(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    taskQueue.push(std::move(task));
  }
  taskQueueCondition.notify_one();
}

void MessagesHandler::sendNotificationAsync(const std::string &method,
                                            const nlohmann::json &params) {
  // Capture params as ordered_json to preserve key order
  nlohmann::ordered_json orderedParams = params;
  submitTask([this, method, orderedParams]() {
    io.sendNotification(method, orderedParams);
  });
}

void MessagesHandler::logMessage(MessageType type, const std::string &message) {
  // Construct params with type first to ensure correct order in JSON output
  nlohmann::ordered_json params = nlohmann::ordered_json::object();
  params["type"] = static_cast<int>(type);
  params["message"] = message;
  sendNotificationAsync("window/logMessage", params);
}

void MessagesHandler::logMessage(MessageType type, lsp::ErrorCode code,
                                 const char *additionalInfo) {
  std::string message = lsp::getErrorMessage(code);
  if (additionalInfo) {
    message += ": ";
    message += additionalInfo;
  }
  logMessage(type, message);
}

void MessagesHandler::workerLoop() {
  while (true) {
    std::unique_lock<std::mutex> lock(taskQueueMutex);
    taskQueueCondition.wait(
        lock, [this] { return !taskQueue.empty() || !workerRunning; });

    if (!workerRunning && taskQueue.empty()) {
      break;
    }

    while (!taskQueue.empty()) {
      auto task = std::move(taskQueue.front());
      taskQueue.pop();
      lock.unlock();

      try {
        task();
      } catch (const std::exception &e) {
        // Log error but continue processing
        // Tasks typically involve HackAssembler operations
        logMessage(MessageType::Error, lsp::ErrorCode::INTERNAL_ERROR,
                   (std::string("HackAssembler error: ") + e.what()).c_str());
      }

      lock.lock();
    }
  }
}
