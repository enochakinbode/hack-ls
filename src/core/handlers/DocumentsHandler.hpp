#pragma once

#include <mutex>
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
    {
      std::lock_guard<std::mutex> lock(mutex);
      uriToDocuments.emplace(params.textDocument.uri, textDocument);
    }
  };

  void onChange(lsp::DidChangeParams params) {

    std::lock_guard<std::mutex> lock(mutex);

    auto it = uriToDocuments.find(params.textDocument.uri);

    if (it == uriToDocuments.end()) {
      lsp::Error error(lsp::ErrorCode::INTERNAL_ERROR, "URI not found");
      throw error;
    }

    TextDocument &textDocument = it->second;
    if (params.textDocument.version > textDocument.version) {
      textDocument.applyChanges(params.contentChanges);
      textDocument.version = params.textDocument.version;
    }
  };

  void onClose(lsp::DidCloseParams params) {

    std::lock_guard<std::mutex> lock(mutex);

    auto it = uriToDocuments.find(params.textDocument.uri);

    if (it == uriToDocuments.end()) {
      lsp::Error error(lsp::ErrorCode::INTERNAL_ERROR, "URI not found");
      throw error;
    }

    uriToDocuments.erase(params.textDocument.uri);
  }

  const std::string &getText(const std::string &uri) {

    std::lock_guard<std::mutex> lock(mutex);

    auto it = uriToDocuments.find(uri);
    if (it == uriToDocuments.end()) {
      lsp::Error error(lsp::ErrorCode::INTERNAL_ERROR,
                       "URI not found in documents");
      throw error;
    }
    TextDocument &textDocument = it->second;
    return textDocument.text;
  }

  const std::unordered_map<std::string, TextDocument> &getDocuments() {
    return uriToDocuments;
  }

private:
  std::unordered_map<std::string, TextDocument> uriToDocuments;
  std::mutex mutex;
};