#pragma once

#include <string>
#include <variant>

namespace lsp {

struct Position {
  int line;
  int character;
};

struct Range {
  Position start;
  Position end;
};

struct TextDocumentIdentifier {
  std::string uri;
};

struct TextDocumentItem : public TextDocumentIdentifier {
  std::string languageId;
  int version;
  std::string text;
};

struct VersionedTextDocumentIdentifier : public TextDocumentIdentifier {
  int version;
};

struct TextDocumentContentChangeEventWithRange {
  Range range;
  std::string text;
};

struct TextDocumentContentChangeEventFull {
  std::string text;
};

using TextDocumentContentChangeEvent =
    std::variant<TextDocumentContentChangeEventWithRange,
                 TextDocumentContentChangeEventFull>;

// TODO: fix this
using LSPAny = int;

enum class TraceValue { Off, Messages, Verbose };

struct WorkspaceFolder {
  std::string uri;
  std::string name;
};

} // namespace lsp
