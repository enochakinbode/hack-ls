#include "lsp/responses.hpp"
#include <nlohmann/json.hpp>

class IRespond {
public:
  virtual void
  respond(const nlohmann::json &id,
          const std::variant<lsp::Result, lsp::Error> &response) noexcept = 0;

  virtual void sendNotification(const std::string &method,
                                const nlohmann::ordered_json &params) = 0;

  virtual ~IRespond() = default;
};