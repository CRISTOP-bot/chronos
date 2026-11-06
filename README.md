# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Stars](https://img.shields.io/github/stars/CRISTOP-bot/chronos?style=for-the-badge&color=ffcb2f)

**Temporal Execution & Reconstruction Engine** — a real reverse-debugging and time-travel analysis tool for programs.

🌐 **Languages:** 🇬🇧 [English](docs/README/README.en.md) · 🇪🇸 [Español](docs/README/README.es.md) · 🇫🇷 [Français](docs/README/README.fr.md) · 🇩🇪 [Deutsch](docs/README/README.de.md) · 🇨🇳 [中文](docs/README/README.zh.md)

CHRONOS records, reconstructs and explores program executions over time. It is not a debugger clone: it is an engineering tool to understand what happened inside a program and recover previous states.

## Features

- **Real recording** via `ptrace`: captures syscalls, process lifecycle and memory maps.
- **Temporal navigation**: forward/backward through an indexed event timeline.
- **Checkpoints** with memory snapshots (regions + bytes).
- **State reconstruction**: variables and memory at any point in time.
- **State Diff**: compare two temporal positions (variables, memory, events).
- **Execution Graph**: visual call tree (GUI + CLI search).
- **Memory Explorer**: addresses, regions, permissions, hex dump.
- **Analytical branches**: create investigation branches from recordings.
- **Binary format** (v2) with zstd compression, CRC32 integrity and versioning.
- **Desktop GUI** (Qt6): dark, minimal, technical interface.
- **CLI** with commands: `trace`, `info`, `maps`, `checkpoints`, `memdiff`, `state`, `search`, `branch`, `export-html`.
- **HTML reports** export.
- **Benchmarks** for storage performance.

## Quick Start

### Build

```bash
git clone https://github.com/CRISTOP-bot/chronos.git
cd chronos
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
```

### Run tests

```bash
cd build && ctest
```

### Trace a program

```bash
./build/tools/chronos_cli trace /tmp/opencode/hello /tmp/opencode/hello.chronos
./build/tools/chronos_cli info /tmp/opencode/hello.chronos
```

### Open the GUI

```bash
./build/gui/chronos_cli /tmp/opencode/hello.chronos  # or chronos_gui
```

## CLI Commands

| Command | Description |
|---|---|
| `demo` | Record a small demo and reopen it |
| `trace <bin> [out]` | Trace a binary with ptrace |
| `info <rec>` | Show recording stats |
| `maps <pid>` | Read /proc maps |
| `checkpoints <rec>` | List checkpoints |
| `memdiff <rec> <a> <b>` | Compare memory between checkpoints |
| `state <rec> <idx>` | Reconstruct variable state at event index |
| `search <rec> <text>` | Search events |
| `branch <src> <dst> <label>` | Create an analytical branch |
| `reconstruct <rec> <id>` | Reconstruct memory from a checkpoint |
| `strings <rec> <id> [filter]` | Extract readable strings from a checkpoint |
| `stats <rec>` | Compute execution metrics |
| `export-html <rec> <out>` | Export HTML report |
| `export-json <rec> <out>` | Export recording to JSON |
| `project-create <dir> <name>` | Create a project |
| `project-add <project.json> <rec>` | Add a recording to a project |
| `project-info <project.json>` | Show project info |

## Project Structure

```
chronos/
├── CMakeLists.txt
├── core/          # common, events, recording, execution, memory, timeline, analysis, storage
├── instrumentation/ # ptrace tracer
├── debugger/      # (planned)
├── gui/           # Qt6 interface
├── tools/         # CLI
├── tests/         # unit + integration tests
├── benchmarks/    # performance benchmarks
├── docs/          # research & architecture
└── brand/         # logo and assets
```

## Architecture

See `docs/00_RESEARCH.md` for the full technical research, architecture, limitations and roadmap.

## Limitations (honest)

- No full deterministic replay yet; reconstruction is event/snapshot based.
- GUI is functional but still growing (Fase 6).
- Initial platform: Linux x86_64.
- Thread support is partial.

## MCP (Model Context Protocol)

CHRONOS ships a minimal MCP server so AI assistants can use it as a tool.

```bash
./mcp.sh /path/to/chronos_cli
```

Exposed tools: `chronos_info`, `chronos_trace`, `chronos_search`, `chronos_state`, `chronos_checkpoints`, `chronos_memdiff`, `chronos_export_html`, `chronos_strings`.

Full review: `docs/MCP_REVIEW.md`. Tested with OpenCode.

## Contributing

See `CONTRIBUTING.md` and `CODE_OF_CONDUCT.md`. Issue/PR templates are in `.github/`.

## License

MIT — see `LICENSE`.