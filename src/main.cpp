#include "common/logging.hpp"
#include "core/handlers/MessageHandler.hpp"
#include "core/transport/io.hpp"
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;
using nlohmann::json;

int main() {

  MessageHandler server;
  while (true) {
    map<string, int> headers;
    //  read headers from stdin
    string line;
    while (getline(cin, line)) {
      if (line.empty() || line == "\r")
        break;

      lsp::parse_header(line, headers);
    }

    if (headers.size() == 0) {
      continue;
    }

    // read content length
    int contentlength = headers["Content-Length"];
    string content(contentlength, '\0');
    cin.read(content.data(), contentlength);

    json message;
    try {
      message = json::parse(content);

    } catch (const std::exception &e) {
      log_error_response(lsp::ErrorCode::PARSE_ERROR, e.what());
      continue;
    }

    if (!server.setMessage(message))
      continue;

    server.run();
  }
  return 0;
}
