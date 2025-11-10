#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/transport/MessageIO.hpp"
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
  std::vector<std::string> previouslyReportedURIs;

  DiagnosticsMessagesMap buildDiagnostics() {

    DiagnosticsMessagesMap uriToDiagnosticMessages;

    const auto &diagnosticsMap = hackAssembler.getDiagnostics();

    for (const auto &entry : diagnosticsMap) {

      const std::string &uri = entry.first;
      Vector *diagnosticVector = entry.second;

      if (diagnosticVector == nullptr || diagnosticVector->size < 1) {

        auto it = std::find(previouslyReportedURIs.begin(),
                            previouslyReportedURIs.end(), uri);

        if (it != previouslyReportedURIs.end()) {
          previouslyReportedURIs.erase(it);

          // Always publish empty diagnostics to clear previous ones
          uriToDiagnosticMessages[uri] = std::vector<lsp::DiagnosticMessage>();
        }

        continue;
      }

      // Add to tracking if not already tracked
      auto it = std::find(previouslyReportedURIs.begin(),
                          previouslyReportedURIs.end(), uri);
      if (it == previouslyReportedURIs.end()) {
        previouslyReportedURIs.push_back(uri);
      }

      std::vector<lsp::DiagnosticMessage> diagnostics;
      diagnostics.reserve(diagnosticVector->size);

      for (int i = 0; i < diagnosticVector->size; i++) {
        auto *diagnostic =
            static_cast<Diagnostic *>(diagnosticVector->items[i]);

        if (diagnostic == nullptr || diagnostic->message == nullptr) {
          continue;
        }

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

    return uriToDiagnosticMessages;
  }

  void
  publishDiagnostics(const std::string &uri,
                     const std::vector<lsp::DiagnosticMessage> &diagnostics) {
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
  }
};
