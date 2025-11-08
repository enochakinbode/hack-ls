#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/structures/TextDocument.hpp"
#include "lsp/errors.hpp"
#include "lsp/params.hpp"

class DocumentsHandler {

public:
  void onOpen(lsp::DidOpenParams params) {

    auto textDocument =
        TextDocument{params.textDocument.uri, params.textDocument.version,
                     params.textDocument.text};
    uriToDococuments.emplace(params.textDocument.uri, textDocument);
  };

  void onChange(lsp::DidChangeParams params) {

    auto it = uriToDococuments.find(params.textDocument.uri);

    if (it == uriToDococuments.end()) {
      lsp::Error error(lsp::ErrorCode::INTERNAL_ERROR, "URI not found",
                       std::nullopt);
      throw error;
    }

    TextDocument &textDocument = it->second;
    if (params.textDocument.version > textDocument.version) {
      textDocument.applyChanges(params.contentChanges);
      textDocument.version = params.textDocument.version;
    }
  };

  void onClose(lsp::DidCloseParams params) {
    uriToDococuments.erase(params.textDocument.uri);
  }

  const std::string &getText(std::string uri) {

    auto it = uriToDococuments.find(uri);
    TextDocument &textDocument = it->second;

    return textDocument.text;
  }

  const std::unordered_map<std::string, TextDocument> getDocuments() {
    return uriToDococuments;
  }

private:
  std::unordered_map<std::string, TextDocument> uriToDococuments;
};