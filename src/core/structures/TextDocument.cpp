#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include "TextDocument.hpp"
#include "lib/utf16_to_utf8.hpp"

size_t TextDocument::positionToOffset(const lsp::Position &position) const {

  // todo: make this more efficient for large documents
  size_t offset = 0;
  for (int i = 0; i < position.line; ++i) {
    offset = text.find('\n', offset);
    if (offset == std::string::npos) {
      // line number out of bounds, clamp to end of text
      return text.size();
    }
    offset += 1; // move past the newline
  }

  // Find the end of the current line
  size_t lineEnd = text.find('\n', offset);
  size_t lineLength =
      (lineEnd == std::string::npos ? text.size() : lineEnd) - offset;

  // Extract the line text
  std::string lineText = text.substr(offset, lineLength);

  // Convert UTF-16 code units to UTF-8 byte offset within the line
  // Clamp to line length in UTF-16 code units
  size_t lineUtf16Length = utf16_to_utf8::getUtf16CodeUnitCount(lineText);
  size_t utf16CharPos = std::min((size_t)position.character, lineUtf16Length);
  size_t utf8CharPos =
      utf16_to_utf8::utf16CodeUnitsToUtf8Offset(lineText, utf16CharPos);

  return offset + utf8CharPos;
}

void TextDocument::applyChanges(
    std::vector<lsp::TextDocumentContentChangeEvent> changes) {

  for (const auto &change : changes) {
    if (std::holds_alternative<lsp::TextDocumentContentChangeEventFull>(
            change)) {

      text = std::get<lsp::TextDocumentContentChangeEventFull>(change).text;
      continue;
    }

    auto rangedChange =
        std::get<lsp::TextDocumentContentChangeEventWithRange>(change);

    // Apply range-based change by replacing text within the specified range
    size_t startOffset = positionToOffset(rangedChange.range.start);
    size_t endOffset = positionToOffset(rangedChange.range.end);

    text.replace(startOffset, endOffset - startOffset, rangedChange.text);
  }
}
