#pragma once

#include <iostream>
#include <map>
#include <string>
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
    documents.emplace(params.textDocument.uri, textDocument);
  };

  void onChange(lsp::DidChangeParams params) {

    auto it = documents.find(params.textDocument.uri);

    if (it == documents.end()) {
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

  std::vector<std::string> &getOpenURIs() { return openURIs; };

  const std::string &getText(std::string uri) {

    auto it = documents.find(uri);
    TextDocument &textDocument = it->second;

    return textDocument.text;
  }

  void onClose() {};

private:
  std::map<std::string, TextDocument> documents;
  std::vector<std::string> openURIs;
};