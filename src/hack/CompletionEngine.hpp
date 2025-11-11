#pragma once

#include <string>
#include <vector>

#include "hack/HackAssembler.hpp"
#include "lsp/params.hpp"
#include "lsp/protocol.hpp"
#include "lsp/responses.hpp"

class CompletionEngine {

public:
  CompletionEngine(HackAssembler &_hackAssembler)
      : hackAssembler(_hackAssembler) {}

  lsp::CompletionResult completion(lsp::CompletionParams &params) {

    // No context or no trigger character - send all completions
    if (!params.context || !params.context->triggerCharacter) {
      return getAllCompletions(params.textDocument.uri);
    }

    const std::string &triggerChar = params.context->triggerCharacter.value();

    // @ triggers symbol completions
    if (triggerChar ==
        protocol::serverDetails::COMPLETION_TRIGGER_CHARACTERS[0]) {
      std::vector<std::string> symbols;
      auto _symbols = hackAssembler.getSymbols(params.textDocument.uri);
      if (_symbols != nullptr) {
        for (int i = 0; i < _symbols->size; i++) {
          symbols.push_back(_symbols->data[i].key);
        }
      }
      return buildCompletions(symbols);
    }

    // = triggers comp completions
    if (triggerChar ==
        protocol::serverDetails::COMPLETION_TRIGGER_CHARACTERS[1]) {
      auto allComps = hackAssembler.getComps();
      return buildCompletions(allComps);
    }

    // ; triggers jump completions
    if (triggerChar ==
        protocol::serverDetails::COMPLETION_TRIGGER_CHARACTERS[2]) {
      auto allJumps = hackAssembler.getJumps();
      return buildCompletions(allJumps);
    }

    // Unknown trigger character - send all
    return getAllCompletions(params.textDocument.uri);
  }

private:
  HackAssembler &hackAssembler;

  lsp::CompletionResult getAllCompletions(const std::string &uri) {
    std::vector<std::string> all;

    // Add symbols
    auto _symbols = hackAssembler.getSymbols(uri);
    if (_symbols != nullptr) {
      for (int i = 0; i < _symbols->size; i++) {
        all.push_back(_symbols->data[i].key);
      }
    }

    // Add dests
    auto dests = hackAssembler.getDests();
    all.insert(all.end(), dests.begin(), dests.end());

    // Add comps
    auto comps = hackAssembler.getComps();
    all.insert(all.end(), comps.begin(), comps.end());

    // Add jumps
    auto jumps = hackAssembler.getJumps();
    all.insert(all.end(), jumps.begin(), jumps.end());

    return buildCompletions(all);
  }

  lsp::CompletionResult buildCompletions(const std::vector<std::string> &list) {
    if (list.empty()) {
      return nullptr;
    }

    std::vector<lsp::CompletionItem> items;
    items.reserve(list.size());

    for (const auto &label : list) {
      lsp::CompletionItem item;
      item.label = label;
      item.kind = lsp::CompletionItemKind::Keyword;
      items.push_back(std::move(item));
    }

    return items;
  }
};