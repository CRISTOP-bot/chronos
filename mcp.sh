#!/usr/bin/env bash
# Launch the CHRONOS MCP server (stdio JSON-RPC).
# Usage: ./mcp.sh [path to chronos_cli]
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
CLI="${1:-$ROOT/build/tools/chronos_cli}"
export CHRONOS_CLI="$CLI"
exec python3 "$ROOT/mcp/mcp_server.py"
