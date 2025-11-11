#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/interfaces/IMessage.hpp"
#include "hack/HackAssembler.hpp"
#include "lsp/messages.hpp"
#include <nlohmann/json.hpp>

extern "C" {
#include "structures.h"
#include "types.h"
}

using DiagnosticsMessagesMap =
    std::unordered_map<std::string, std::vector<lsp::DiagnosticMessage>>;

class DiagnosticsEngine {
public:
  DiagnosticsEngine(HackAssembler &_hackAssembler, IMessage &_io)
      : hackAssembler(_hackAssembler), io(_io) {};

  void report() {
    DiagnosticsMessagesMap uriToDiagnosticMessages;
    buildDiagnostics(uriToDiagnosticMessages);

    for (const auto &entry : uriToDiagnosticMessages) {
      publishDiagnostics(entry.first, entry.second);
    }
  }

private:
  HackAssembler &hackAssembler;
  IMessage &io;
  std::unordered_map<std::string, std::vector<lsp::DiagnosticMessage>>
      lastPublishedByUri;

  void buildDiagnostics(DiagnosticsMessagesMap &uriToDiagnosticMessages) {

    const auto &uriToDiagnosticsVector = hackAssembler.getAllDiagnostics();

    for (const auto &entry : uriToDiagnosticsVector) {

      const std::string &uri = entry.first;
      Vector *diagnosticVector = entry.second;

      std::vector<lsp::DiagnosticMessage> diagnostics;
      diagnostics.reserve(diagnosticVector->size);

      for (int i = 0; i < diagnosticVector->size; i++) {
        auto *diagnostic =
            static_cast<Diagnostic *>(diagnosticVector->items[i]);

        lsp::DiagnosticMessage message;
        message.line = std::max(diagnostic->line - 1, 0);
        message.character = 0;
        message.message = diagnostic->message;
        message.severity = lsp::Severity::Error;
        diagnostics.push_back(std::move(message));
      }

      // Sort diagnostics by line number, then by character position
      std::sort(diagnostics.begin(), diagnostics.end(),
                [](const lsp::DiagnosticMessage &lhs,
                   const lsp::DiagnosticMessage &rhs) {
                  if (lhs.line == rhs.line) {
                    return lhs.character < rhs.character;
                  }
                  return lhs.line < rhs.line;
                });

      uriToDiagnosticMessages[uri] = std::move(diagnostics);
    }
  }

  void
  publishDiagnostics(const std::string &uri,
                     const std::vector<lsp::DiagnosticMessage> &diagnostics) {
    // Skip sending if identical to last published for this URI
    auto sameAsLast = [&]() -> bool {
      auto it = lastPublishedByUri.find(uri);
      if (it == lastPublishedByUri.end()) {
        return false;
      }
      const auto &prev = it->second;
      if (prev.size() != diagnostics.size()) {
        return false;
      }
      for (size_t i = 0; i < diagnostics.size(); ++i) {
        const auto &a = prev[i];
        const auto &b = diagnostics[i];
        if (a.line != b.line || a.character != b.character ||
            a.message != b.message || a.severity != b.severity) {
          return false;
        }
      }
      return true;
    }();

    if (sameAsLast) {
      return;
    }

    nlohmann::ordered_json params;
    params["uri"] = uri;

    auto diagnosticsArray = nlohmann::ordered_json::array();

    for (const auto &diagnostic : diagnostics) {

      nlohmann::ordered_json diagnosticJson;
      diagnosticJson["range"] = {
          {"start",
           {{"line", diagnostic.line}, {"character", diagnostic.character}}},
          {"end",
           {{"line", diagnostic.line}, {"character", diagnostic.character}}}};

      diagnosticJson["severity"] = static_cast<int>(diagnostic.severity);
      diagnosticJson["source"] = "hack-assembler";
      diagnosticJson["message"] = diagnostic.message;

      diagnosticsArray.push_back(std::move(diagnosticJson));
    }

    params["diagnostics"] = std::move(diagnosticsArray);

    io.sendNotification("textDocument/publishDiagnostics", params);
    lastPublishedByUri[uri] = diagnostics;
  }
};
