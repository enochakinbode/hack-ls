#pragma once

class IServerState {
public:
  virtual bool isInitialized() = 0;
  virtual void onInitialize() = 0;
  virtual void onShutdown() = 0;
  virtual void onExit() = 0;
  virtual bool shouldExit() = 0;
  virtual bool isShutdownRequested() const = 0;
  virtual void allowNotifications() = 0;
  virtual bool isNotficationnAllowed() const = 0;

  virtual ~IServerState() = default;
};