#pragma once

#include <string>
#include <vector>

#include "lsp/types.hpp"

struct TextDocument {
  size_t positionToOffset(const lsp::Position &position) const;

public:
  std::string uri;
  int version;
  std::string text;

  TextDocument(std::string uri, int version, std::string text)
      : uri(uri), version(version), text(text) {}

  void applyChanges(std::vector<lsp::TextDocumentContentChangeEvent>);
};
