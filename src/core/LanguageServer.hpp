#pragma once

#include "core/handlers/IServerInitState.hpp"
#include "core/handlers/MessagesHandler.hpp"
#include "core/transport/MessageIO.hpp"

class LanguageServer : public IServerIntailizationState {

public:
  LanguageServer() : messagesHandler(*this, io), running(true) {};

  void start();

  bool isInitailized() override { return initialized; };
  void onInitialize() override { initialized = true; };

private:
  MessageIO io;
  MessagesHandler messagesHandler;
  bool running;
  bool initialized = false;
};