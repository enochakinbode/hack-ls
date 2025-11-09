#pragma once

#include <string>
#include <vector>

#include "DiagnosticsEngine.hpp"
#include "HackAssembler.hpp"
#include "core/handlers/DocumentsHandler.hpp"
#include "core/transport/MessageIO.hpp"
#include "hack/CompletionEngine.hpp"
#include "hack/HoverEngine.hpp"
#include "lsp/params.hpp"
#include "lsp/responses.hpp"

class HackManager {
public:
  HackManager(DocumentsHandler &_documentsHandler, IRespond &_io)
      : hackAssembler(_documentsHandler), diagnosticsEngine(hackAssembler, _io),
        completionEngine(hackAssembler),
        hoverEngine(hackAssembler, _documentsHandler) {}

  void processDocument(const std::string uri) {
    // Step 1: Run assembler
    std::vector<std::string> uris = {uri};
    hackAssembler.run(uris);

    // Step 2: Publish diagnostics
    diagnosticsEngine.report();
  }

  lsp::CompletionResult completion(lsp::CompletionParams &params) {
    return completionEngine.completion(params);
  }

  lsp::HoverResult hover(lsp::HoverParams &params) {
    return hoverEngine.hover(params);
  }

  void freeURIResult(const std::string &uri) {
    hackAssembler.freeURIResult(uri);
  }

  void freeAllResults() { hackAssembler.freeAllResults(); }

private:
  HackAssembler hackAssembler;
  DiagnosticsEngine diagnosticsEngine;
  CompletionEngine completionEngine;
  HoverEngine hoverEngine;
};
