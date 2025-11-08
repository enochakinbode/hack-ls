#!/bin/bash

# Script to test LSP server with multiple .asm files
# Usage: ./tests/test.sh | ./build/hack-ls

# Get the absolute path to the tests directory
TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASM_DIR="$TESTS_DIR/tests/asm"

# Configuration: Set to "string" to send IDs as strings, "int" for integers
REQUEST_ID_TYPE="${REQUEST_ID_TYPE:-int}"  # Default to int, can be overridden

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

# Function to create didOpen message
create_did_open() {
    local file_path="$1"
    local filename=$(basename "$file_path")
    local encoded_path=$(url_encode "$file_path")
    local uri="file://$encoded_path"
    
    # Read file content and use jq to properly escape JSON
    if command -v jq &> /dev/null; then
        local content=$(cat "$file_path")
        # Use jq to create compact JSON and ensure no trailing newline
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
            }' | jq -c .)
        # Remove any trailing newline that jq might add
        json_body="${json_body%$'\n'}"
        json_body="${json_body%$'\r'}"
    else
        # Fallback: manual escaping (less robust)
        local content=$(cat "$file_path" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\r/\\r/g' | sed 's/\t/\\t/g')
        local json_body="{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\",\"params\":{\"textDocument\":{\"uri\":\"$uri\",\"languageId\":\"asm\",\"version\":1,\"text\":\"$content\"}}}"
    fi
    
    # Calculate Content-Length exactly (bytes in the JSON string)
    # Use printf to get exact byte count, accounting for UTF-8
    local content_length=$(printf "%s" "$json_body" | wc -c | tr -d ' ')
    
    # Output with proper LSP format (no trailing newline)
    printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json_body"
    
    # Show status after sending, then give time for response
    echo -e "\033[33mdidOpen: --> $filename\033[0m" >&2
    sleep 0.1
}

# Function to create didChange message
create_did_change() {
    local content_file="$1"  # File to read content from
    local uri_file="${2:-$1}"  # File to use for URI (defaults to content_file)
    
    # Validate file paths
    if [ -z "$content_file" ] || [ ! -f "$content_file" ]; then
        echo -e "\033[31mError: Content file '$content_file' not found\033[0m" >&2
        return 1
    fi
    
    if [ -z "$uri_file" ] || [ ! -f "$uri_file" ]; then
        echo -e "\033[31mError: URI file '$uri_file' not found\033[0m" >&2
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
            '{
                "jsonrpc": "2.0",
                "method": "textDocument/didChange",
                "params": {
                    "textDocument": {
                        "uri": $uri,
                        "version": 2
                    },
                    "contentChanges": [{
                        "text": $content
                    }]
                }
            }' | jq -c . | tr -d '\n')
    else
        local content=$(cat "$content_file" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\r/\\r/g' | sed 's/\t/\\t/g')
        local json_body="{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\",\"params\":{\"textDocument\":{\"uri\":\"$uri\",\"version\":2},\"contentChanges\":[{\"text\":\"$content\"}]}}"
    fi
    
    # Remove any trailing newlines
    json_body="${json_body%$'\n'}"
    json_body="${json_body%$'\r'}"
    local content_length=$(printf "%s" "$json_body" | wc -c | tr -d ' ')
    printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json_body"
    
    # Show status after sending, then give time for response
    echo -e "\033[33mdidChange: --> $filename\033[0m" >&2
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
    
    # Remove any trailing newlines
    json_body="${json_body%$'\n'}"
    json_body="${json_body%$'\r'}"
    local content_length=$(printf "%s" "$json_body" | wc -c | tr -d ' ')
    printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json_body"
    
    # Show status after sending, then give time for response
    echo -e "\033[33mcompletion: --> $filename (line:$line, char:$character, trigger:'$trigger_char')\033[0m" >&2
    sleep 0.1
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
                        "workspace": {
                            "applyEdit": true,
                            "workspaceEdit": {
                                "documentChanges": true
                            },
                            "configuration": true
                        },
                        "textDocument": {
                            "synchronization": {
                                "didSave": true
                            },
                            "completion": {
                                "completionItem": {
                                    "snippetSupport": true
                                }
                            },
                            "hover": {},
                            "definition": {},
                            "references": {},
                            "documentHighlight": {},
                            "documentSymbol": {},
                            "codeAction": {
                                "codeActionLiteralSupport": {
                                    "codeActionKind": {
                                        "valueSet": ["quickfix", "refactor", "source", "source.organizeImports"]
                                    }
                                }
                            },
                            "formatting": {},
                            "rangeFormatting": {},
                            "onTypeFormatting": {},
                            "rename": {
                                "prepareSupport": true
                            },
                            "publishDiagnostics": {
                                "tagSupport": {
                                    "valueSet": [1, 2]
                                }
                            }
                        },
                        "window": {
                            "workDoneProgress": true,
                            "showMessage": {}
                        },
                        "general": {
                            "staleRequestSupport": {
                                "retryOnContentModified": []
                            }
                        }
                    },
                    "clientInfo": {
                        "name": "My-LSP-Client",
                        "version": "1.0.0"
                    },
                    "initializationOptions": {}
                }
            }' | jq -c .)
        # Remove any trailing newline that jq might add
        json_body="${json_body%$'\n'}"
        json_body="${json_body%$'\r'}"
    else
        # Fallback: manual JSON construction
        # root_uri already includes file://, so use it directly
        json_body="{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"processId\":$process_id,\"rootUri\":\"$root_uri\",\"capabilities\":{\"workspace\":{\"applyEdit\":true,\"workspaceEdit\":{\"documentChanges\":true},\"configuration\":true},\"textDocument\":{\"synchronization\":{\"didSave\":true},\"completion\":{\"completionItem\":{\"snippetSupport\":true}},\"hover\":{},\"definition\":{},\"references\":{},\"documentHighlight\":{},\"documentSymbol\":{},\"codeAction\":{\"codeActionLiteralSupport\":{\"codeActionKind\":{\"valueSet\":[\"quickfix\",\"refactor\",\"source\",\"source.organizeImports\"]}}},\"formatting\":{},\"rangeFormatting\":{},\"onTypeFormatting\":{},\"rename\":{\"prepareSupport\":true},\"publishDiagnostics\":{\"tagSupport\":{\"valueSet\":[1,2]}}},\"window\":{\"workDoneProgress\":true,\"showMessage\":{}},\"general\":{\"staleRequestSupport\":{\"retryOnContentModified\":[]}}},\"clientInfo\":{\"name\":\"My-LSP-Client\",\"version\":\"1.0.0\"},\"initializationOptions\":{}}}"
    fi
    
    # Calculate Content-Length exactly (bytes in the JSON string)
    local content_length=$(printf "%s" "$json_body" | wc -c | tr -d ' ')
    
    # Output with proper LSP format using \r\n
    printf "Content-Length: %d\r\n\r\n%s" "$content_length" "$json_body"
}

# Send initialize once
echo -e "\033[33minitialize: --> starting\033[0m" >&2
ROOT_URI="file://$(cd "$TESTS_DIR" && pwd)"
create_init "$ROOT_URI"

# Process each .asm file
request_id=2
for asm_file in "$ASM_DIR"/*.asm; do
    if [ ! -f "$asm_file" ]; then
        continue
    fi
    
    filename=$(basename "$asm_file")
    
    # # Skip Pong.asm for now
    if [ "$filename" = "Pong.asm" ]; then
        continue
    fi
    
    # Get absolute path for URI
    abs_path=$(cd "$(dirname "$asm_file")" && pwd)/$(basename "$asm_file")
    
    # Skip Add2.asm and Max2.asm - they'll be used for didChange
    if [ "$filename" = "Add2.asm" ] || [ "$filename" = "Max2.asm" ]; then
        continue
    fi
    
    # For Add.asm and Max.asm: didOpen -> didChange -> completions
    create_did_open "$abs_path"
    echo ""
    
    # Check if corresponding *2.asm file exists and send didChange
    base_name="${filename%.asm}"
    change_file="$ASM_DIR/${base_name}2.asm"
    if [ -f "$change_file" ]; then
        change_abs_path=$(cd "$(dirname "$change_file")" && pwd)/$(basename "$change_file")
        create_did_change "$change_abs_path" "$abs_path"
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

# Keep pipe open for interactive use
cat

