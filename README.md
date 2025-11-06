# Hack (Nand2Tetris) Language Server Implementation

A minimal Language Server (LS) implementation in C++ for the Hack assembly language from the Nand2Tetris course.

## Architecture

```
src/
├── lsp/                    # LSP protocol layer
│   ├── types.hpp          # Position, Range, TextDocument types
│   ├── messages.hpp       # RequestMessage, NotificationMessage
│   ├── params.hpp         # Method parameters (Initialize, DidOpen, DidChange)
│   ├── responses.hpp      # Response structures and ServerCapabilities
│   ├── errors.hpp         # Error codes and Error class
│   └── protocol.hpp       # Server capabilities and protocol details
├── core/                   # Server Implementation
│   ├── handlers/
│   │   ├── MessageHandler     # Routes and processes LSP messages
│   │   ├── DocumentHandler    # Handles document lifecycle (open/change)
│   │   └── IServerInitState   # Server initialization state interface
│   ├── structures/
│   │   └── TextDocument       # Document state and incremental updates
│   ├── transport/
│   │   └── MessageIO          # I/O layer for LSP protocol (stdin/stdout)
│   ├── LanguageServer.hpp    # Main server class
│   └── LanguageServer.cpp
├── util/
│   └── logging.hpp            # Logging utilities
└── main.cpp                   # Entry point: LS message loop over stdin/stdout
```


## Current Capabilities

- ✅ Initialize handshake
- ✅ Document synchronization (open/change)
- ✅ Incremental text updates
- 🚧 Definition provider (advertised, not implemented)
- ❌ Hover, completion, diagnostics (not yet)

## Build & Run

```bash
cmake -B build && cmake --build build
./build/bin/hack-language-server
```
