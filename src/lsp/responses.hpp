#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lsp/errors.hpp"
#include "lsp/types.hpp"
#include <nlohmann/json.hpp>

namespace lsp {

typedef nlohmann::ordered_json InitializeResult;

enum class CompletionItemKind {
  Text = 1,
  Variable = 6,
  Unit = 11,
  Value = 12,
  Keyword = 14,
};

struct CompletionItem {
  std::string label;
  std::optional<CompletionItemKind> kind;
  std::optional<std::string> detail;
  std::optional<std::string> documentation;
};

struct CompletionList {
  bool isIncomplete;
  std::vector<CompletionItem> items;
};

using CompletionResult =
    std::variant<std::nullptr_t, std::vector<CompletionItem>, CompletionList>;

struct HoverItem {
  std::string contents;
  Range range;
};

using HoverResult = std::variant<nullptr_t, HoverItem>;

using Result = std::variant<std::nullptr_t, InitializeResult, CompletionResult,
                            HoverResult>;

struct Response {
  int contentLength;
  std::string body;
};

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const CompletionItem &item) {
  j["label"] = item.label;
  if (item.kind.has_value())
    j["kind"] = static_cast<int>(item.kind.value());
  if (item.detail.has_value())
    j["detail"] = item.detail.value();
  if (item.documentation.has_value())
    j["documentation"] = item.documentation.value();
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const CompletionList &list) {
  j["isIncomplete"] = list.isIncomplete;
  nlohmann::ordered_json itemsArray = nlohmann::ordered_json::array();
  for (const auto &item : list.items) {
    nlohmann::ordered_json itemJson;
    to_json(itemJson, item);
    itemsArray.push_back(itemJson);
  }
  j["items"] = itemsArray;
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const CompletionResult &result) {

  if (std::holds_alternative<std::nullptr_t>(result)) {
    j = nullptr;

  } else if (std::holds_alternative<std::vector<CompletionItem>>(result)) {
    const auto &items = std::get<std::vector<CompletionItem>>(result);
    nlohmann::ordered_json itemsArray = nlohmann::ordered_json::array();
    for (const auto &item : items) {
      nlohmann::ordered_json itemJson;
      to_json(itemJson, item);
      itemsArray.push_back(itemJson);
    }
    j = itemsArray;

  } else if (std::holds_alternative<CompletionList>(result)) {
    to_json(j, std::get<CompletionList>(result));
  }
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const HoverResult &result) {

  if (std::holds_alternative<std::nullptr_t>(result)) {
    j = nullptr;
  } else {
    auto res = std::get<HoverItem>(result);

    j = {{"contents", res.contents},
         {"range",
          {{"start",
            {{"line", res.range.start.line},
             {"character", res.range.start.character}}},
           {"end",
            {{"line", res.range.end.line},
             {"character", res.range.end.character}}}}}};
  }
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const Result &result) {

  if (std::holds_alternative<std::nullptr_t>(result)) {
    j = nullptr;
  } else if (std::holds_alternative<InitializeResult>(result)) {
    j = std::get<InitializeResult>(result);
  } else if (std::holds_alternative<CompletionResult>(result)) {
    to_json(j, std::get<CompletionResult>(result));
  } else
    to_json(j, std::get<HoverResult>(result));
}

inline void to_json(nlohmann::basic_json<nlohmann::ordered_map> &j,
                    const Error &error) {

  j["code"] = static_cast<int>(error.code);
  j["message"] = error.what();
  if (error.data.has_value()) {
    j["data"] = error.data.value();
  }
}

} // namespace lsp
