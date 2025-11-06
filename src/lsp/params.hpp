#pragma once

#include "types.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <vector>

namespace lsp {

using ProgressToken = std::variant<int, std::string>;

template <typename T> struct ProgressParams {
  ProgressToken token;
  T value;
};

struct WorkDoneProgressParams {

  std::optional<ProgressToken> workDoneToken;
};

using DocumentUri = std::string;

struct ClientInfo {
  std::string name;
  std::optional<std::string> version;
};

// struct RegularExpressionsClientCapabilities { std::optional<std::string>
// engine; std::optional<std::string> version; }; struct
// MarkdownClientCapabilities { std::optional<std::string> parser;
// std::optional<std::string> version; std::optional<std::vector<std::string>>
// allowedTags; }; struct WorkspaceEditClientCapabilities {}; struct
// DidChangeConfigurationClientCapabilities {}; struct
// DidChangeWatchedFilesClientCapabilities {}; struct
// WorkspaceSymbolClientCapabilities {}; struct ExecuteCommandClientCapabilities
// {}; struct SemanticTokensWorkspaceClientCapabilities {}; struct
// CodeLensWorkspaceClientCapabilities {}; struct
// InlineValueWorkspaceClientCapabilities {}; struct
// InlayHintWorkspaceClientCapabilities {}; struct
// DiagnosticWorkspaceClientCapabilities {}; struct
// TextDocumentClientCapabilities {}; struct NotebookDocumentClientCapabilities
// {}; struct ShowMessageRequestClientCapabilities {}; struct
// ShowDocumentClientCapabilities {}; struct FileOperationsClientCapabilities {
// std::optional<bool> dynamicRegistration; std::optional<bool> didCreate;
// std::optional<bool> willCreate; std::optional<bool> didRename;
// std::optional<bool> willRename; std::optional<bool> didDelete;
// std::optional<bool> willDelete; };

struct WorkspaceClientCapabilities {
  std::optional<bool> applyEdit;
  // std::optional<WorkspaceEditClientCapabilities> workspaceEdit;
  // std::optional<DidChangeConfigurationClientCapabilities>
  // didChangeConfiguration;
  // std::optional<DidChangeWatchedFilesClientCapabilities>
  // didChangeWatchedFiles; std::optional<WorkspaceSymbolClientCapabilities>
  // symbol; std::optional<ExecuteCommandClientCapabilities> executeCommand;
  // std::optional<bool> workspaceFolders;
  // std::optional<bool> configuration;
  // std::optional<SemanticTokensWorkspaceClientCapabilities> semanticTokens;
  // std::optional<CodeLensWorkspaceClientCapabilities> codeLens;
  // std::optional<FileOperationsClientCapabilities> fileOperations;
  // std::optional<InlineValueWorkspaceClientCapabilities> inlineValue;
  // std::optional<InlayHintWorkspaceClientCapabilities> inlayHint;
  // std::optional<DiagnosticWorkspaceClientCapabilities> diagnostics;
};

// struct WindowClientCapabilities { std::optional<bool> workDoneProgress;
// std::optional<ShowMessageRequestClientCapabilities> showMessage;
// std::optional<ShowDocumentClientCapabilities> showDocument; };

// struct StaleRequestSupportClientCapability { bool cancel;
// std::vector<std::string> retryOnContentModified; };

struct GeneralClientCapabilities {
  // std::optional<StaleRequestSupportClientCapability> staleRequestSupport;
  // std::optional<RegularExpressionsClientCapabilities> regularExpressions;
  // std::optional<MarkdownClientCapabilities> markdown;
  std::optional<std::vector<std::string>> positionEncodings;
};

struct ClientCapabilities {
  std::optional<WorkspaceClientCapabilities> workspace;
  // std::optional<TextDocumentClientCapabilities> textDocument;
  // std::optional<NotebookDocumentClientCapabilities> notebookDocument;
  // std::optional<WindowClientCapabilities> window;
  std::optional<GeneralClientCapabilities> general;
  // std::optional<LSPAny> experimental;
};

struct InitializeParams : public WorkDoneProgressParams {

  std::variant<int, std::nullptr_t> processId;
  std::optional<ClientInfo> clientInfo;
  std::optional<std::string> locale;

  // @deprecated in favour of `rootUri`.
  // optional<string> rootPath;

  std::variant<DocumentUri, std::nullptr_t> rootUri;

  std::optional<LSPAny> initializationOptions;
  ClientCapabilities capabilities;
  std::optional<TraceValue> trace;
  // std::optional<std::vector<WorkspaceFolder>> workspaceFolders;
};

struct DidOpenParams {
  TextDocumentItem textDocument;
};

struct DidChangeParams {
  VersionedTextDocumentIdentifier textDocument;
  std::vector<TextDocumentContentChangeEvent> contentChanges;
};

inline void from_json(const nlohmann::json &j, lsp::ClientInfo &ci) {
  j.at("name").get_to(ci.name);
  if (j.contains("version"))
    ci.version = j.at("version").get<std::string>();
}

inline void from_json(const nlohmann::json &j, lsp::WorkspaceFolder &wf) {
  j.at("uri").get_to(wf.uri);
  j.at("name").get_to(wf.name);
}

inline void from_json(const nlohmann::json &j, lsp::ClientCapabilities &c) {
  if (j.contains("workspace") && j.at("workspace").is_object()) {
    const auto &w = j.at("workspace");
    if (w.contains("applyEdit"))
      c.workspace.emplace().applyEdit = w.at("applyEdit").get<bool>();
  }
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
} // namespace lsp
