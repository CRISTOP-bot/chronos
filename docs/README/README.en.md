# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**Temporal Execution & Reconstruction Engine** — a real reverse-debugging and time-travel analysis tool for programs.

CHRONOS records, reconstructs and explores program executions over time. It is not a debugger clone: it is an engineering tool to understand what happened inside a program and recover previous states.

🌐 **Languages:** 🇬🇧 [English](README.en.md) · 🇪🇸 [Español](README.es.md) · 🇫🇷 [Français](README.fr.md) · 🇩🇪 [Deutsch](README.de.md) · 🇨🇳 [中文](README.zh.md)

## Features
- Real recording via ptrace.
- Temporal navigation, checkpoints, state diff.
- Memory explorer with hex dump and strings.
- Execution graph and call stack.
- Qt6 GUI and CLI.
- Binary format v2 with zstd compression.

## Quick Start
```bash
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## CLI Commands
trace, info, maps, checkpoints, memdiff, state, search, branch, export-html, export-json, strings, reconstruct, stats, project-create/add/info.

## License
MIT.