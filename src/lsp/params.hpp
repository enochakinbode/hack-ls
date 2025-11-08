#pragma once

#include "types.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lsp {

using DocumentUri = std::string;

struct ClientInfo {
  std::string name;
  std::optional<std::string> version;
};

struct GeneralClientCapabilities {
  std::optional<std::vector<std::string>> positionEncodings;
};

struct ClientCapabilities {
  // std::optional<TextDocumentClientCapabilities> textDocument;
  std::optional<GeneralClientCapabilities> general;
  // std::optional<LSPAny> experimental;
};

struct InitializeParams {

  std::variant<int, std::nullptr_t> processId;
  std::optional<ClientInfo> clientInfo;
  std::optional<std::string> locale;

  // @deprecated in favour of `rootUri`.
  // optional<string> rootPath;

  std::variant<DocumentUri, std::nullptr_t> rootUri;

  std::optional<LSPAny> initializationOptions;
  ClientCapabilities capabilities;
  std::optional<TraceValue> trace;
};

struct DidOpenParams {
  TextDocumentItem textDocument;
};

struct DidChangeParams {
  VersionedTextDocumentIdentifier textDocument;
  std::vector<TextDocumentContentChangeEvent> contentChanges;
};

struct DidCloseParams {
  TextDocumentIdentifier textDocument;
};

struct CompletionContext {
  enum class TriggerKind {
    Invoked = 1,
    TriggerCharacter = 2,
    TriggerForIncompleteCompletions = 3
  };
  TriggerKind triggerKind;
  std::optional<std::string> triggerCharacter;
};

struct CompletionParams {
  TextDocumentIdentifier textDocument;
  Position position;
  std::optional<CompletionContext> context;
};

inline void from_json(const nlohmann::json &j, lsp::ClientInfo &ci) {
  j.at("name").get_to(ci.name);
  if (j.contains("version"))
    ci.version = j.at("version").get<std::string>();
}

inline void from_json(const nlohmann::json &j, lsp::ClientCapabilities &c) {

  if (j.contains("general") && j.at("general").is_object()) {
    const auto &g = j.at("general");
    if (g.contains("positionEncodings") &&
        g.at("positionEncodings").is_array()) {
      std::vector<std::string> encs;
      for (const auto &e : g.at("positionEncodings")) {
        if (e.is_string())
          encs.push_back(e.get<std::string>());
      }
      if (!encs.empty()) {
        c.general.emplace();
        c.general->positionEncodings = std::move(encs);
      }
    }
  }
}

inline void from_json(const nlohmann::json &j, lsp::InitializeParams &p) {
  // processId: number or null
  if (j.contains("processId") && !j.at("processId").is_null()) {
    p.processId = j.at("processId").get<int>();
  } else {
    p.processId = nullptr;
  }

  if (j.contains("clientInfo"))
    p.clientInfo = j.at("clientInfo").get<lsp::ClientInfo>();
  if (j.contains("locale") && j.at("locale").is_string())
    p.locale = j.at("locale").get<std::string>();

  // rootUri: string or null
  if (j.contains("rootUri") && !j.at("rootUri").is_null()) {
    p.rootUri = j.at("rootUri").get<std::string>();
  } else {
    p.rootUri = nullptr;
  }

  // capabilities (only needed fields)
  p.capabilities = j.at("capabilities").get<lsp::ClientCapabilities>();

  // trace (optional; minimal)
  if (j.contains("trace") && j.at("trace").is_string()) {
    const auto v = j.at("trace").get<std::string>();
    if (v == "off")
      p.trace = lsp::TraceValue::Off;
    else if (v == "messages")
      p.trace = lsp::TraceValue::Messages;
    else if (v == "verbose")
      p.trace = lsp::TraceValue::Verbose;
  }
}

inline void from_json(const nlohmann::json &j,
                      lsp::DidOpenParams &didOpenParams) {
  j.at("textDocument").at("uri").get_to(didOpenParams.textDocument.uri);
  j.at("textDocument")
      .at("languageId")
      .get_to(didOpenParams.textDocument.languageId);
  j.at("textDocument").at("version").get_to(didOpenParams.textDocument.version);
  j.at("textDocument").at("text").get_to(didOpenParams.textDocument.text);
}

inline void from_json(const nlohmann::json &j,
                      lsp::DidChangeParams &didChangeParams) {
  j.at("textDocument").at("uri").get_to(didChangeParams.textDocument.uri);
  j.at("textDocument")
      .at("version")
      .get_to(didChangeParams.textDocument.version);

  for (auto &change : j.at("contentChanges")) {
    if (change.contains("range")) {

      int start_line, start_character, end_line, end_character;
      lsp::Position start, end;

      change.at("range").at("start").at("line").get_to<int>(start_line);
      change.at("range")
          .at("start")
          .at("character")
          .get_to<int>(start_character);

      change.at("range").at("end").at("line").get_to<int>(end_line);
      change.at("range").at("end").at("character").get_to<int>(end_character);

      start = lsp::Position{start_line, start_character};
      end = lsp::Position{end_line, end_character};

      auto range = lsp::Range{start, end};

      std::string text;
      change.at("text").get_to<std::string>(text);
      didChangeParams.contentChanges.push_back(
          lsp::TextDocumentContentChangeEventWithRange{
              range,
              text,
          });
      continue;
    }

    std::string text;
    change.at("text").get_to<std::string>(text);
    didChangeParams.contentChanges.push_back(
        lsp::TextDocumentContentChangeEventFull{text});
  }
}

inline void from_json(const nlohmann::json &j,
                      lsp::DidCloseParams &didCloseParams) {
  j.at("textDocument").at("uri").get_to(didCloseParams.textDocument.uri);
}

inline void from_json(const nlohmann::json &j, lsp::CompletionContext &ctx) {

  int triggerKindInt = j.at("triggerKind").get<int>();
  if (triggerKindInt == 1) {
    ctx.triggerKind = lsp::CompletionContext::TriggerKind::Invoked;
  } else if (triggerKindInt == 2) {
    ctx.triggerKind = lsp::CompletionContext::TriggerKind::TriggerCharacter;
  } else if (triggerKindInt == 3) {
    ctx.triggerKind =
        lsp::CompletionContext::TriggerKind::TriggerForIncompleteCompletions;
  }

  if (j.contains("triggerCharacter") && j.at("triggerCharacter").is_string()) {
    ctx.triggerCharacter = j.at("triggerCharacter").get<std::string>();
  }
}

inline void from_json(const nlohmann::json &j, lsp::CompletionParams &params) {
  // textDocument
  j.at("textDocument").at("uri").get_to(params.textDocument.uri);

  // position
  int line, character;
  j.at("position").at("line").get_to<int>(line);
  j.at("position").at("character").get_to<int>(character);
  params.position = lsp::Position{line, character};

  // context (optional)
  if (j.contains("context") && j.at("context").is_object()) {
    params.context = j.at("context").get<lsp::CompletionContext>();
  }
}
} // namespace lsp
