#pragma once

#include "core/handlers/MessagesHandler.hpp"
#include "core/interfaces/IServerState.hpp"
#include "core/transport/MessageIO.hpp"

class LanguageServer : public IServerState {

public:
  LanguageServer() : messagesHandler(*this, io), running(true) {};

  void start();

  bool isInitialized() override { return initialized; }
  void onInitialize() override { initialized = true; }
  void onShutdown() override { shutdownRequested = true; }
  void onExit() override { running = false; }
  bool shouldExit() override { return !running; }
  bool isShutdownRequested() const override { return shutdownRequested; }

private:
  MessageIO io;
  MessagesHandler messagesHandler;
  bool running;
  bool initialized = false;
  bool shutdownRequested = false;
};