#pragma once

#include <map>
#include <string>
#include <vector>

class HackAssembler;
class IRespond;

enum class Severity : int { Error = 1, Warning = 2, Information = 3, Hint = 4 };

struct DiagnosticMessage {
  int line = 0;
  int character = 0;
  std::string message;
  Severity severity = Severity::Error;
};

using DiagnosticsMessagesMap =
    std::map<std::string, std::vector<DiagnosticMessage>>;

class DiagnosticsEngine {
public:
  DiagnosticsEngine(HackAssembler &_hackAssembler, IRespond &_io)
      : hackAssembler(_hackAssembler), io(_io) {};

  void report();

private:
  HackAssembler &hackAssembler;
  IRespond &io;

  DiagnosticsMessagesMap buildDiagnostics();
  void publishDiagnostics(const std::string &uri,
                          const std::vector<DiagnosticMessage> &diagnostics);
};
