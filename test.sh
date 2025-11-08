#!/bin/bash

# Script to generate LSP test messages for multiple .asm files
# Usage: ./test.sh | your-lsp-server
# Or: ./test.sh --shutdown | your-lsp-server  (to test proper shutdown)

set -euo pipefail

# Get the absolute path to the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASM_DIR="$SCRIPT_DIR/tests/asm"

# Configuration
REQUEST_ID_TYPE="${REQUEST_ID_TYPE:-int}"  # Default to int, can be overridden
SHUTDOWN="${SHUTDOWN:-false}"  # Set to true to test shutdown sequence
VERBOSE="${VERBOSE:-true}"     # Set to false to suppress status messages

# Function to URL encode file path (simple version - handles spaces and common chars)
url_encode() {
    local string="$1"
    # Return empty if input is empty
    if [ -z "$string" ]; then
        echo -n ""
        return
    fi
    # For file:// URIs, we mainly need to handle spaces
    # Use Python if available for proper encoding, otherwise simple replacement
    if command -v python3 &> /dev/null; then
        python3 -c "import urllib.parse; print(urllib.parse.quote('$string', safe='/'))"
    else
        # Simple fallback: just replace spaces (most common case)
        echo -n "$string" | sed 's/ /%20/g'
    fi
}

# Helper function to send LSP message
send_lsp_message() {
    local json_body="$1"
    local content_length=$(printf "%s" "$json_body" | wc -c | tr -d ' ')
    printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json_body"
}

# Helper function to log status
log_status() {
    if [ "$VERBOSE" = "true" ]; then
        echo -e "\033[33m$1\033[0m" >&2
    fi
}

# Function to create didOpen message
create_did_open() {
    local file_path="$1"
    local filename=$(basename "$file_path")
    local encoded_path=$(url_encode "$file_path")
    local uri="file://$encoded_path"
    
    if [ ! -f "$file_path" ]; then
        log_status "Error: File '$file_path' not found"
        return 1
    fi
    
    # Read file content and use jq to properly escape JSON
    if command -v jq &> /dev/null; then
        local content=$(cat "$file_path")
        local json_body=$(jq -n \
            --arg uri "$uri" \
            --arg content "$content" \
            '{
                "jsonrpc": "2.0",
                "method": "textDocument/didOpen",
                "params": {
                    "textDocument": {
                        "uri": $uri,
                        "languageId": "asm",
                        "version": 1,
                        "text": $content
                    }
                }
            }' | jq -c . | tr -d '\n')
    else
        # Fallback: manual escaping (less robust)
        local content=$(cat "$file_path" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\r/\\r/g' | sed 's/\t/\\t/g')
        local json_body="{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"$uri\",\"languageId\":\"asm\",\"version\":1,\"text\":\"$content\"}}}"
    fi
    
    send_lsp_message "$json_body"
    log_status "didOpen: --> $filename"
    sleep 0.1
}

# Function to create didChange message
create_did_change() {
    local content_file="$1"  # File to read content from
    local uri_file="${2:-$1}"  # File to use for URI (defaults to content_file)
    local version="${3:-2}"   # Document version
    
    # Validate file paths
    if [ ! -f "$content_file" ]; then
        log_status "Error: Content file '$content_file' not found"
        return 1
    fi
    
    if [ ! -f "$uri_file" ]; then
        log_status "Error: URI file '$uri_file' not found"
        return 1
    fi
    
    local filename=$(basename "$uri_file")
    local encoded_path=$(url_encode "$uri_file")
    local uri="file://$encoded_path"
    
    if command -v jq &> /dev/null; then
        local content=$(cat "$content_file")
        local json_body=$(jq -n \
            --arg uri "$uri" \
            --arg content "$content" \
            --argjson version "$version" \
            '{
                "jsonrpc": "2.0",
                "method": "textDocument/didChange",
                "params": {
                    "textDocument": {
                        "uri": $uri,
                        "version": $version
                    },
                    "contentChanges": [{
                        "text": $content
                    }]
                }
            }' | jq -c . | tr -d '\n')
    else
        local content=$(cat "$content_file" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\r/\\r/g' | sed 's/\t/\\t/g')
        local json_body="{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"$uri\",\"version\":$version},\"contentChanges\":[{\"text\":\"$content\"}]}}"
    fi
    
    send_lsp_message "$json_body"
    log_status "didChange: --> $filename (version: $version)"
    sleep 0.1
}

# Function to create didClose message
create_did_close() {
    local file_path="$1"
    local filename=$(basename "$file_path")
    local encoded_path=$(url_encode "$file_path")
    local uri="file://$encoded_path"
    
    if [ ! -f "$file_path" ]; then
        log_status "Error: File '$file_path' not found"
        return 1
    fi
    
    local json_body
    if command -v jq &> /dev/null; then
        json_body=$(jq -n \
            --arg uri "$uri" \
            '{
                "jsonrpc": "2.0",
                "method": "textDocument/didClose",
                "params": {
                    "textDocument": {
                        "uri": $uri
                    }
                }
            }' | jq -c . | tr -d '\n')
    else
        json_body="{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didClose\",\"params\":{\"textDocument\":{\"uri\":\"$uri\"}}}"
    fi
    
    send_lsp_message "$json_body"
    log_status "didClose: --> $filename"
    sleep 0.1
}

# Function to create completion request
create_completion() {
    local file_path="$1"
    local filename=$(basename "$file_path")
    local line="$2"
    local character="$3"
    local trigger_char="$4"
    local request_id="$5"
    local encoded_path=$(url_encode "$file_path")
    local uri="file://$encoded_path"
    
    # Format ID based on REQUEST_ID_TYPE variable
    local id_value
    if [ "$REQUEST_ID_TYPE" = "string" ]; then
        id_value="\"$request_id\""
    else
        id_value="$request_id"
    fi
    
    local json_body
    if [ -n "$trigger_char" ]; then
        json_body="{\"jsonrpc\":\"2.0\",\"id\":$id_value,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"$uri\"},\"position\":{\"line\":$line,\"character\":$character},\"context\":{\"triggerKind\":2,\"triggerCharacter\":\"$trigger_char\"}}}"
    else
        json_body="{\"jsonrpc\":\"2.0\",\"id\":$id_value,\"method\":\"textDocument/completion\",\"params\":{\"textDocument\":{\"uri\":\"$uri\"},\"position\":{\"line\":$line,\"character\":$character},\"context\":{\"triggerKind\":1}}}"
    fi
    
    send_lsp_message "$json_body"
    log_status "completion: --> $filename (line:$line, char:$character, trigger:'$trigger_char')"
    sleep 0.1
}

# Function to create shutdown request
create_shutdown() {
    local request_id="${1:-999}"
    local id_value
    if [ "$REQUEST_ID_TYPE" = "string" ]; then
        id_value="\"$request_id\""
    else
        id_value="$request_id"
    fi
    
    local json_body="{\"jsonrpc\":\"2.0\",\"id\":$id_value,\"method\":\"shutdown\",\"params\":null}"
    send_lsp_message "$json_body"
    log_status "shutdown: --> request sent"
    sleep 0.2  # Give server time to process shutdown
}

# Function to create exit notification
create_exit() {
    local json_body="{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":null}"
    send_lsp_message "$json_body"
    log_status "exit: --> notification sent"
}

# Function to create initialize request
create_init() {
    local root_uri="$1"
    local process_id="${2:-12345}"
    
    if command -v jq &> /dev/null; then
        local json_body=$(jq -n \
            --arg root_uri "$root_uri" \
            --argjson process_id "$process_id" \
            '{
                "jsonrpc": "2.0",
                "id": 1,
                "method": "initialize",
                "params": {
                    "processId": $process_id,
                    "rootUri": $root_uri,
                    "capabilities": {
                        "textDocument": {
                            "completion": {
                                "completionItem": {
                                    "snippetSupport": true
                                }
                            }
                        }
                    },
                    "clientInfo": {
                        "name": "test-client",
                        "version": "1.0.0"
                    }
                }
            }' | jq -c . | tr -d '\n')
    else
        # Fallback: manual JSON construction
        json_body="{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"processId\":$process_id,\"rootUri\":\"$root_uri\",\"capabilities\":{\"textDocument\":{\"completion\":{\"completionItem\":{\"snippetSupport\":true}}}},\"clientInfo\":{\"name\":\"test-client\",\"version\":\"1.0.0\"}}}"
    fi
    
    send_lsp_message "$json_body"
    log_status "initialize: --> starting"
    sleep 0.2  # Give server time to respond
}

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        --shutdown)
            SHUTDOWN=true
            shift
            ;;
        --no-shutdown)
            SHUTDOWN=false
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --quiet)
            VERBOSE=false
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --shutdown        Test proper shutdown sequence"
            echo "  --no-shutdown     Keep connection open (default)"
            echo "  --verbose         Show status messages (default)"
            echo "  --quiet           Suppress status messages"
            echo "  --help, -h        Show this help message"
            echo ""
            echo "Environment variables:"
            echo "  REQUEST_ID_TYPE   Request ID type: 'int' or 'string' (default: int)"
            echo "                    Use 'string' for string IDs, 'int' for numeric IDs"
            echo "  SHUTDOWN          Set to 'true' to test shutdown"
            echo "  VERBOSE           Set to 'false' to suppress output"
            echo ""
            echo "Examples:"
            echo "  $0 | ./hack-ls                    # Run with default settings"
            echo "  REQUEST_ID_TYPE=string $0 | ./hack-ls  # Use string request IDs"
            echo ""
            echo "Note: For incremental changes testing, use test_incremental.sh"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            echo "Use --help for usage information" >&2
            exit 1
            ;;
    esac
done

# Validate ASM directory exists
if [ ! -d "$ASM_DIR" ]; then
    echo "Error: ASM directory not found: $ASM_DIR" >&2
    exit 1
fi


# Function to generate all test messages
generate_test_messages() {
    # Send initialize request
    ROOT_URI="file://$SCRIPT_DIR"
    create_init "$ROOT_URI"
    echo ""

    # Send initialized notification
    if command -v jq &> /dev/null; then
        init_notif=$(jq -nc '{"jsonrpc":"2.0","method":"initialized","params":{}}' | tr -d '\n')
    else
        init_notif='{"jsonrpc":"2.0","method":"initialized","params":{}}'
    fi
    send_lsp_message "$init_notif"
    log_status "initialized: --> notification"
    echo ""
    sleep 0.1

    # Process each .asm file
    request_id=2
    opened_files=()
    shutdown_request_id=999  # Use a high number for shutdown to avoid conflicts

    for asm_file in "$ASM_DIR"/*.asm; do
        if [ ! -f "$asm_file" ]; then
            continue
        fi
        
        filename=$(basename "$asm_file")
        
        # Skip Pong.asm (too large for testing)
        if [ "$filename" = "Pong.asm" ]; then
            continue
        fi
        
        # Skip Add2.asm and Max2.asm - they'll be used for didChange
        if [ "$filename" = "Add2.asm" ] || [ "$filename" = "Max2.asm" ]; then
            continue
        fi
        
        # Get absolute path for URI
        abs_path=$(cd "$(dirname "$asm_file")" && pwd)/$(basename "$asm_file")
        opened_files+=("$abs_path")
        
        # For Add.asm and Max.asm: didOpen -> didChange -> completions
        create_did_open "$abs_path"
        echo ""
        
        # Check if corresponding *2.asm file exists and send didChange
        base_name="${filename%.asm}"
        change_file="$ASM_DIR/${base_name}2.asm"
        if [ -f "$change_file" ]; then
            change_abs_path=$(cd "$(dirname "$change_file")" && pwd)/$(basename "$change_file")
            create_did_change "$change_abs_path" "$abs_path" 2
            echo ""
        fi
        
        # Send completion requests with different trigger characters
        # @ trigger (symbols) - position after @ on line 8
        create_completion "$abs_path" 7 1 "@" $request_id
        request_id=$((request_id + 1))
        echo ""
        
        # = trigger (comps) - position after = on line 9 (D=A)
        create_completion "$abs_path" 8 2 "=" $request_id
        request_id=$((request_id + 1))
        echo ""
        
        # ; trigger (jumps) - position after ; (if we had a jump instruction)
        # For Max.asm which has jumps, use line 12 (D;JGT)
        if [ "$filename" = "Max.asm" ]; then
            create_completion "$abs_path" 11 2 ";" $request_id
            request_id=$((request_id + 1))
            echo ""
        fi
        
        # Manual trigger (no trigger character - all completions)
        # Position in middle of a line
        create_completion "$abs_path" 7 2 "" $request_id
        request_id=$((request_id + 1))
        echo ""
    done

    # Test didClose for all opened files
    if [ "$SHUTDOWN" = "false" ]; then
        for file_path in "${opened_files[@]}"; do
            create_did_close "$file_path"
            echo ""
        done
    fi

    # Send shutdown and exit if requested
    if [ "$SHUTDOWN" = "true" ]; then
        log_status "Sending shutdown request..."
        create_shutdown "$shutdown_request_id"
        echo ""
        sleep 0.2
        log_status "Sending exit notification..."
        create_exit
        echo ""
    else
        log_status "Keeping connection open (use --shutdown to test proper shutdown)"
        # Keep pipe open for interactive use
        cat
    fi
}

# Generate and output test messages to stdout
# User can pipe this to their LSP server: ./test.sh | your-lsp-server
generate_test_messages

