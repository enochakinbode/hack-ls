#pragma once

class IServerIntailizationState {
public:
  virtual bool isInitailized() = 0;
  virtual void onInitialize() = 0;

  virtual ~IServerIntailizationState() = default;
};