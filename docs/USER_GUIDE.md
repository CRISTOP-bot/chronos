# CHRONOS — User Guide

## What is CHRONOS?
CHRONOS is a temporal execution & reconstruction engine for reverse debugging. It records real program executions, lets you navigate time, inspect previous states, and compare differences.

## Build
```bash
git clone https://github.com/CRISTOP-bot/chronos.git
cd chronos
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## Quick Start (CLI)
```bash
# Trace a program
./tools/chronos_cli trace /tmp/opencode/hello /tmp/opencode/hello.chronos

# Show stats
./tools/chronos_cli info /tmp/opencode/hello.chronos

# Reconstruct state at event 3
./tools/chronos_cli state /tmp/opencode/d2.chronos 3

# Compare memory between checkpoints
./tools/chronos_cli memdiff /tmp/opencode/hello.chronos 1 2

# Search events
./tools/chronos_cli search /tmp/opencode/hello.chronos write

# Extract strings
./tools/chronos_cli strings /tmp/opencode/hello.chronos 1 hello

# Export HTML report
./tools/chronos_cli export-html /tmp/opencode/hello.chronos /tmp/opencode/report.html

# Export JSON
./tools/chronos_cli export-json /tmp/opencode/hello.chronos /tmp/opencode/out.json

# Create an analytical branch
./tools/chronos_cli branch /tmp/opencode/hello.chronos /tmp/opencode/branch.chronos investigation-1
```

## GUI
```bash
./gui/chronos_gui /tmp/opencode/hello.chronos
```
- Welcome screen → Open Recording.
- Tabs: Events, Memory, Calls, Diff.
- Memory tab: select checkpoint → view regions, hex dump, strings filter.
- Calls tab: call stack and execution graph.
- Diff tab: set A/B and compare.

## MCP (AI integration)
```bash
./mcp.sh /path/to/chronos_cli
```
Tools: chronos_info, chronos_trace, chronos_search, chronos_state, chronos_checkpoints, chronos_memdiff, chronos_export_html, chronos_strings.

## Project Management
```bash
./tools/chronos_cli project-create ./myproj "My Project"
./tools/chronos_cli project-add ./myproj/project.json /tmp/opencode/hello.chronos
./tools/chronos_cli project-info ./myproj/project.json
```

## Benchmarks
```bash
./benchmarks/chronos_bench
```

## Limitations
- No full deterministic replay yet.
- GUI still growing.
- Platform: Linux x86_64.