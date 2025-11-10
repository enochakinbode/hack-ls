#pragma once

#include <unordered_map>
#include <vector>

#include "core/handlers/DocumentsHandler.hpp"

extern "C" {
#include "assembler.h"
#include "structures.h"
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

      setUpTables(result.dests, result.comps, result.jumps);
      uriToAssembleResult[uris[i]] = result;
      free(source);
    }
  };

  std::unordered_map<std::string, Vector *> getDiagnostics() const {
    std::unordered_map<std::string, Vector *> diagnostics;

    for (const auto &entry : uriToAssembleResult) {
      diagnostics[entry.first] = entry.second.diagnostics;
    }

    return diagnostics;
  };

  Map *getSymbols(const std::string &uri) {
    return uriToAssembleResult[uri].symbols;
  };

  const std::vector<std::string> &getDests() const { return dests; };
  const std::vector<std::string> &getComps() const { return comps; };
  const std::vector<std::string> &getJumps() const { return jumps; };

  void freeURIResult(const std::string &uri) {
    auto it = uriToAssembleResult.find(uri);
    if (it == uriToAssembleResult.end()) {
      return;
    }

    AssemblerResult__free(&it->second, assemblerConfig);
    uriToAssembleResult.erase(it);
  }

  void freeAllResults() {
    for (auto &entry : uriToAssembleResult) {
      AssemblerResult__free(&entry.second, assemblerConfig);
    }
    uriToAssembleResult.clear();
  }

private:
  AssemblerConfig assemblerConfig = {0, 0};
  DocumentsHandler &documentHandler;
  std::unordered_map<std::string, AssemblerResult> uriToAssembleResult;
  std::vector<std::string> dests, comps, jumps;
  bool isSetUp = false;

  void setUpTables(Map *_dests, Map *_comps, Map *_jumps) {
    if (isSetUp)
      return;

    extractKeys(_dests, dests);
    extractKeys(_comps, comps);
    extractKeys(_jumps, jumps);

    isSetUp = true;
  }

  void extractKeys(Map *map, std::vector<std::string> &output) {
    if (map == nullptr)
      return;

    output.reserve(map->size);
    for (int i = 0; i < map->size; i++) {
      output.push_back(map->data[i].key);
    }
  }
};