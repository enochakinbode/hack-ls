#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "core/handlers/DocumentsHandler.hpp"
#include "core/interfaces/IServerState.hpp"
#include "core/transport/MessageIO.hpp"
#include "hack/HackManager.hpp"
#include "lsp/errors.hpp"
#include "lsp/messages.hpp"
#include "lsp/responses.hpp"
#include <nlohmann/json.hpp>

class MessagesHandler {

public:
  MessagesHandler(IServerState &_server, IRespond &_io)
      : server(_server), io(_io), hackManager(documentsHandler, _io),
        workerRunning(true),
        workerThread(&MessagesHandler::workerLoop, this) {};

  ~MessagesHandler() {
    {
      std::lock_guard<std::mutex> lock(taskQueueMutex);
      workerRunning = false;
    }
    taskQueueCondition.notify_one();
    if (workerThread.joinable()) {
      workerThread.join();
    }
    // Free all assembler results on shutdown to prevent memory leaks
    hackManager.freeAllResults();
  }

  int process(nlohmann::json &_message);

  // Submit a task to be executed on the worker thread
  void submitTask(std::function<void()> task);

  // Send a logMessage notification through the worker thread
  void logMessage(MessageType type, const std::string &message);
  // Overload that uses ErrorCode to get the default message
  void logMessage(MessageType type, lsp::ErrorCode code,
                  const char *additionalInfo = nullptr);

private:
  IServerState &server;
  IRespond &io;
  DocumentsHandler documentsHandler;
  HackManager hackManager;

  // Worker thread management
  void workerLoop();
  bool workerRunning;
  std::queue<std::function<void()>> taskQueue;
  std::mutex taskQueueMutex;
  std::condition_variable taskQueueCondition;
  std::thread workerThread;

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

      logMessage(MessageType::Error, lsp::ErrorCode::INVALID_MESSAGE, e.what());
      return 0;
    }
  }

  void send_response(const nlohmann::json &id,
                     const lsp::Result &result) noexcept {
    io.respond(id, std::variant<lsp::Result, lsp::Error>(result));
  }

  void send_response(const nlohmann::json &id, lsp::ErrorCode code,
                     const char *message = nullptr,
                     const std::optional<nlohmann::json> &data = std::nullopt) {

    std::string errorMessage =
        message ? std::string(message) : lsp::getErrorMessage(code);
    lsp::Error error(code, errorMessage, data);
    io.respond(id, std::variant<lsp::Result, lsp::Error>(error));
  }

  void sendNotificationAsync(const std::string &method,
                             const nlohmann::ordered_json &params);
};