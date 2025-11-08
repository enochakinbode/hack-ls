#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "HackAssembler.hpp"
#include "core/transport/MessageIO.hpp"
#include <nlohmann/json.hpp>

extern "C" {
#include "structures.h"
#include "types.h"
}

enum class Severity : int { Error = 1, Warning = 2, Information = 3, Hint = 4 };

struct DiagnosticMessage {
  int line = 0;
  int character = 0;
  std::string message;
  Severity severity = Severity::Error;
};

using DiagnosticsMessagesMap =
    std::unordered_map<std::string, std::vector<DiagnosticMessage>>;

class DiagnosticsEngine {
public:
  DiagnosticsEngine(HackAssembler &_hackAssembler, IRespond &_io)
      : hackAssembler(_hackAssembler), io(_io) {};

  void report() {
    auto diagnostics = buildDiagnostics();

    for (const auto &entry : diagnostics) {
      publishDiagnostics(entry.first, entry.second);
    }
  }

private:
  HackAssembler &hackAssembler;
  IRespond &io;

  DiagnosticsMessagesMap buildDiagnostics() {
    DiagnosticsMessagesMap uriToDiagnosticMessages;

    const std::unordered_map<std::string, Vector *> &diagnosticsMap =
        hackAssembler.getDiagnostics();

    for (const auto &entry : diagnosticsMap) {
      const std::string &uri = entry.first;
      Vector *diagnosticVector = entry.second;

      if (diagnosticVector->size < 1)
        continue;

      std::vector<DiagnosticMessage> diagnostics;

      diagnostics.reserve(diagnosticVector->size);

      for (int i = 0; i < diagnosticVector->size; i++) {
        auto *diagnostic =
            static_cast<Diagnostic *>(diagnosticVector->items[i]);

        if (diagnostic == nullptr || diagnostic->message == nullptr) {
          continue;
        }

        DiagnosticMessage message;
        message.line = std::max(diagnostic->line - 1, 0);
        message.character = 0;
        message.message = diagnostic->message;
        message.severity = Severity::Error;
        diagnostics.push_back(std::move(message));
      }

      // Sort diagnostics by line number, then by character position
      std::sort(diagnostics.begin(), diagnostics.end(),
                [](const DiagnosticMessage &lhs, const DiagnosticMessage &rhs) {
                  if (lhs.line == rhs.line) {
                    return lhs.character < rhs.character;
                  }
                  return lhs.line < rhs.line;
                });

      uriToDiagnosticMessages[uri] = std::move(diagnostics);
    }

    return uriToDiagnosticMessages;
  }

  void publishDiagnostics(const std::string &uri,
                          const std::vector<DiagnosticMessage> &diagnostics) {
    nlohmann::ordered_json params;
    params["uri"] = uri;

    nlohmann::ordered_json diagnosticsArray = nlohmann::ordered_json::array();

    for (const auto &diagnostic : diagnostics) {
      nlohmann::ordered_json diagnosticJson;
      diagnosticJson["range"]["start"]["line"] = diagnostic.line;
      diagnosticJson["range"]["start"]["character"] = diagnostic.character;
      diagnosticJson["range"]["end"]["line"] = diagnostic.line;
      diagnosticJson["range"]["end"]["character"] = diagnostic.character;
      diagnosticJson["severity"] = static_cast<int>(diagnostic.severity);
      diagnosticJson["source"] = "hack-assembler";
      diagnosticJson["message"] = diagnostic.message;

      diagnosticsArray.push_back(std::move(diagnosticJson));
    }

    params["diagnostics"] = std::move(diagnosticsArray);

    io.sendNotification("textDocument/publishDiagnostics", params);
  }
};
