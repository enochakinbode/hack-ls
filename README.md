# Hack (Nand2Tetris) Language Server

A Language Server Protocol (LSP) implementation in C++ for the Hack assembly language from the Nand2Tetris course.

## Features

### ✅ Implemented
- **Initialize handshake** - Full LSP initialization with server capabilities
- **Document synchronization** - Open, change, and close notifications
- **Incremental text updates** - Efficient document change tracking
- **Completion provider** - Code completion for Hack assembly instructions
- **Diagnostics** - Real-time error reporting from the Hack assembler
- **Logging** - Server-side logging via `window/logMessage` notifications
- **Worker thread system** - Asynchronous document processing
- **Error handling** - Comprehensive error responses and logging


### ❌ Not Yet Implemented
- Hover provider
- Go to definition
- Find references
- Code formatting

## Architecture

The codebase is organized into three main layers:
- **`lsp/`** - LSP protocol layer (types, messages, params, responses, errors)
- **`core/`** - Server implementation (handlers, document structures, transport I/O)
- **`hack/`** - Hack-specific functionality (assembler integration, diagnostics, completion)

## Key Components

### Worker Thread System
The server uses a dedicated worker thread for asynchronous operations:
- Document processing (assembling and diagnostics)
- Notification sending (logMessage, publishDiagnostics)
- Prevents blocking the main message processing loop

### Completion Engine
Provides intelligent code completion for:
- A-instructions (`@symbol`)
- C-instructions (dest=comp;jump)
- Symbols and labels
- Triggered by `@`, `=`, `;` characters

### Diagnostics Engine
Converts Hack assembler errors into LSP diagnostics:
- Real-time error reporting
- Line and character position mapping
- Error severity classification

## Build & Run

### Prerequisites
- C++20 compatible compiler (clang++ or g++)
- CMake 3.16 or higher
- HackAssembler frontend (included in `external/HackAssembler/`)

### Build
```bash
cmake -B build && cmake --build build
```

### Run
```bash
./build/bin/hack-ls --stdio
```
The server communicates via stdin/stdout using the LSP protocol.

## Testing

The project includes a test script (`test.sh`) that exercises the LSP server with multiple Hack assembly files.

```bash
./test.sh | ./build/bin/hack-ls
```

The test script will:
- Initialize the LSP server
- Open multiple `.asm` files from `tests/asm/` directory
- Test incremental document changes (`didChange`)
- Test completion requests with various trigger characters (`@`, `=`, `;`)
- Test manual completion triggers

### Test Files

Place your Hack assembly test files in `tests/asm/` directory. The script will automatically process all `.asm` files found there.

## Development

See [TODO.txt](TODO.txt) for current development priorities and known issues.

## License

MIT
