#pragma once

#include <string>
#include <vector>

#include "DiagnosticsEngine.hpp"
#include "HackAssembler.hpp"
#include "core/handlers/DocumentsHandler.hpp"
#include "core/transport/MessageIO.hpp"

class HackManager {
public:
  HackManager(DocumentsHandler &_documentsHandler, IRespond &_io)
      : hackAssembler(_documentsHandler),
        diagnosticsEngine(hackAssembler, _io) {}

  void processDocument(const std::string &uri) {
    // Step 1: Run assembler
    std::vector<std::string> uris = {uri};
    hackAssembler.run(uris);

    // Step 2: Publish diagnostics
    diagnosticsEngine.report();
  }

  // Future: async versions (to be implemented with threads/queue)
  // void processDocumentAsync(const std::string &uri);
  // void processDocumentsAsync(const std::vector<std::string> &uris);

private:
  HackAssembler hackAssembler;
  DiagnosticsEngine diagnosticsEngine;
};
