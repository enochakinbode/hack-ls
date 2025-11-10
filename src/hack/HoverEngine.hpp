#pragma once

#include "core/handlers/DocumentsHandler.hpp"
#include "hack/HackAssembler.hpp"
#include "lib/utf16_to_utf8.hpp"
#include "lsp/params.hpp"
#include "lsp/responses.hpp"
#include "lsp/types.hpp"
#include <cctype>
#include <string>
#include <utility>

class HoverEngine {
public:
  HoverEngine(HackAssembler &_hackAssembler,
              DocumentsHandler &_documentsHandler)
      : hackAssembler(_hackAssembler), documentsHandler(_documentsHandler) {};

  lsp::HoverResult hover(lsp::HoverParams &params) {

    auto &text = documentsHandler.getText(params.textDocument.uri);
    auto res = getWordUnderCursor(params.position, text);

    if (!res.first.starts_with("@"))
      return lsp::HoverResult(nullptr);

    auto symbols = hackAssembler.getSymbols(params.textDocument.uri);

    int val = 0;

    for (int i = 0; i < symbols->size; i++) {
      if (res.first.substr(1) == symbols->data[i].key) {
        val = symbols->data[i].value;
        break;
      }
    }

    std::string contents = res.first + " = " + std::to_string(val) +
                           "\n\n✨ This symbol sets the A and M registers to " +
                           std::to_string(val);
    return lsp::HoverItem{.contents = contents, .range = res.second};
  };

private:
  HackAssembler &hackAssembler;
  DocumentsHandler &documentsHandler;

  std::pair<std::string, lsp::Range>
  getWordUnderCursor(lsp::Position &pos, const std::string &text) {

    // Extract the line at pos.line
    size_t offset = 0;
    for (int i = 0; i < pos.line; ++i) {
      offset = text.find('\n', offset);
      if (offset == std::string::npos) {
        return std::make_pair(std::string(""), lsp::Range{pos, pos});
      }
      offset += 1;
    }

    // Find the end of the current line
    size_t lineEnd = text.find('\n', offset);
    size_t lineLength =
        (lineEnd == std::string::npos ? text.size() : lineEnd) - offset;

    // Extract the line text
    std::string lineText = text.substr(offset, lineLength);

    // Convert UTF-16 character position to UTF-8 byte offset
    size_t lineUtf16Length = utf16_to_utf8::getUtf16CodeUnitCount(lineText);
    size_t utf16CharPos = std::min((size_t)pos.character, lineUtf16Length);
    size_t utf8CharPos =
        utf16_to_utf8::utf16CodeUnitsToUtf8Offset(lineText, utf16CharPos);

    // Find word boundaries
    size_t wordStart = utf8CharPos;
    size_t wordEnd = utf8CharPos;

    // Find start of word (alphanumeric or @)
    while (wordStart > 0 &&
           (std::isalnum(lineText[wordStart - 1]) ||
            lineText[wordStart - 1] == '@' || lineText[wordStart - 1] == '_')) {
      wordStart--;
    }

    // Find end of word
    while (wordEnd < lineText.size() &&
           (std::isalnum(lineText[wordEnd]) || lineText[wordEnd] == '@' ||
            lineText[wordEnd] == '_')) {
      wordEnd++;
    }

    // Extract the word
    std::string word = lineText.substr(wordStart, wordEnd - wordStart);

    // Convert UTF-8 byte positions back to UTF-16 character positions for range
    size_t startUtf16 =
        utf16_to_utf8::getUtf16CodeUnitCountUpToOffset(lineText, wordStart);
    size_t endUtf16 =
        utf16_to_utf8::getUtf16CodeUnitCountUpToOffset(lineText, wordEnd);

    lsp::Range range = {{pos.line, static_cast<int>(startUtf16)},
                        {pos.line, static_cast<int>(endUtf16)}};

    return std::make_pair(word, range);
  };
};