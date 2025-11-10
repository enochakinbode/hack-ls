#pragma once

#include <nlohmann/json.hpp>

namespace protocol {

namespace serverDetails {

// Protocol version information
constexpr const char *LSP_VERSION = "3.17.0";
constexpr const char *SERVER_NAME = "hack-ls";
constexpr const char *SERVER_VERSION = "0.1.0";

// capabilities
constexpr const char *POSITION_ENCODING = "utf-16"; // LSP standard uses UTF-16
constexpr bool TEXT_DOCUMENT_OPEN_CLOSE = true;
constexpr int TEXT_DOCUMENT_SYNC = 2;
constexpr bool WILL_SAVE = false;
constexpr bool WILL_SAVE_WAIT_UNTIL = false;
constexpr bool SAVE_INCLUDE_TEXT = false;

// Features supported
constexpr bool SUPPORTS_HOVER = true;
constexpr bool SUPPORTS_COMPLETION = true;

// Completion options
constexpr bool COMPLETION_RESOLVE_PROVIDER = false;
constexpr const char *COMPLETION_TRIGGER_CHARACTERS[] = {"@", "=", ";"};

inline nlohmann::ordered_json to_json() {
  nlohmann::ordered_json j;

  j = {{"capabilities",
        {{"positionEncoding", POSITION_ENCODING},

         {"textDocumentSync",
          {{"openClose", TEXT_DOCUMENT_OPEN_CLOSE},
           {"change", TEXT_DOCUMENT_SYNC},
           {"willSave", WILL_SAVE_WAIT_UNTIL},
           {"willSaveWaitUntil", WILL_SAVE_WAIT_UNTIL},
           {"save", {{"includeText", SAVE_INCLUDE_TEXT}}}}},

         {"hoverProvider", SUPPORTS_HOVER},

         {"completionProvider",
          {{"resolveProvider", COMPLETION_RESOLVE_PROVIDER},
           {"triggerCharacters",
            {COMPLETION_TRIGGER_CHARACTERS[0], COMPLETION_TRIGGER_CHARACTERS[1],
             COMPLETION_TRIGGER_CHARACTERS[2]}}}}}},

       {"serverInfo", {{"name", SERVER_NAME}, {"version", SERVER_VERSION}}}};

  return j;
}
} // namespace serverDetails
} // namespace protocol