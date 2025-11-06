#pragma once

#include "core/handlers/IServerInitState.hpp"
#include "core/handlers/MessageHandler.hpp"
#include "core/transport/MessageIO.hpp"

class LanguageServer : public IServerIntailizationState {

public:
  LanguageServer() : messageHandler(*this, io), running(true) {};

  void start();

  bool isInitailized() override { return initialized; };
  void onInitialize() override { initialized = true; };

private:
  MessageIO io;
  MessageHandler messageHandler;
  bool running;
  bool initialized = false;
};