#pragma once

#include "core/handlers/DocumentsHandler.hpp"
#include <map>
#include <vector>

extern "C" {
#include "assembler.h"
#include "types.h"
}

class HackAssembler {

public:
  HackAssembler(DocumentsHandler &_documentHandler)
      : documentHandler(_documentHandler) {};

  void run(const std::vector<std::string> &uris) {

    for (uint i = 0; i < uris.size(); i++) {

      auto it = uriToAssembleResult.find(uris[i]);
      if (it != uriToAssembleResult.end()) {
        freeURIResult(uris[i]);
      }

      const char *text = documentHandler.getText(uris[i]).c_str();
      char *source = strdup(text);
      AssemblerResult result = assemble(source, assemblerConfig);
      uriToAssembleResult[uris[i]] = result;
      free(source);
    }
  };

  std::map<std::string, Vector *> getDiagnostics() {
    std::map<std::string, Vector *> diagnostics;

    for (const auto &entry : uriToAssembleResult) {
      diagnostics[entry.first] = entry.second.diagnostics;
    }

    return diagnostics;
  }

private:
  AssemblerConfig assemblerConfig = {0, 0};
  DocumentsHandler &documentHandler;
  std::map<std::string, AssemblerResult> uriToAssembleResult;

  void freeURIResult(const std::string &uri) {

    auto it = uriToAssembleResult.find(uri);
    if (it == uriToAssembleResult.end()) {
      return;
    }

    AssemblerResult__free(&it->second, assemblerConfig);
    uriToAssembleResult.erase(it);
  }
};