# MCP Review — CHRONOS

## Overview
CHRONOS ships a minimal Model Context Protocol (MCP) server (`mcp/mcp_server.py`) launched via `mcp.sh`. It exposes CHRONOS CLI capabilities to AI assistants over stdio JSON-RPC.

## Tested With
- OpenCode (MCP client) — all tools invoked successfully.
- Python integration test: `tests/test_mcp.py` (8/8 tools).

## Tools Exposed
1. `chronos_info` — recording stats.
2. `chronos_trace` — trace a binary.
3. `chronos_search` — search events by text.
4. `chronos_state` — reconstruct variable state.
5. `chronos_checkpoints` — list checkpoints.
6. `chronos_memdiff` — compare memory between checkpoints.
7. `chronos_export_html` — export HTML report.
8. `chronos_strings` — extract readable strings.

## Test Results
- initialize: OK
- tools/list: 8 tools returned.
- All tools/call: OK (see test_mcp.py output).
- No errors or broken pipes in normal usage.

## Usage
```bash
./mcp.sh /path/to/chronos_cli
```
Set `CHRONOS_CLI` env var if needed.

## Limitations
- Server is minimal (no streaming, no resources/prompts).
- Requires `chronos_cli` binary built.
- JSON-RPC over stdio only.