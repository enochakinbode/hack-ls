#include <cstring>
#include <iostream>

#include "core/LanguageServer.hpp"
#include "lsp/protocol.hpp"

struct ParseArgsResult {
  bool stdio;
  bool version;
};

ParseArgsResult parse_args(int argc, char **args) {

  ParseArgsResult res = {false, false};
  for (int i = 0; i < argc; i++) {
    if (strcmp(args[i], "--stdio") == 0)
      res.stdio = true;
    if (strcmp(args[i], "--version") == 0)
      res.version = true;
  }

  if (argc < 2 || (!res.stdio && !res.version)) {
    std::cerr << "run with --stdio\n" << std::flush;
    std::exit(1);
  }

  return res;
}

int main(int argc, char **args) {

  auto res = parse_args(argc, args);

  if (res.version) {
    std::cout << "Hack Language Server\n";
    std::cout << "version: " << protocol::serverDetails::SERVER_VERSION
              << std::endl;
    return 0;

  } else if (res.stdio) {
    LanguageServer server;
    server.start();
  }

  return 0;
}