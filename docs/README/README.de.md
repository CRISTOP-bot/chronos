# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**Temporale Ausführungs- und Rekonstruktions-Engine** — echtes Reverse-Debugging- und Zeitreise-Analysetool.

CHRONOS zeichnet, rekonstruiert und erkundet Programmausführungen über die Zeit. Es ist kein Debugger-Klon: es ist ein Ingenieurwerkzeug, um zu verstehen, was in einem Programm passiert ist, und frühere Zustände wiederherzustellen.

🌐 **Sprachen:** 🇬🇧 [English](README.en.md) · 🇪🇸 [Español](README.es.md) · 🇫🇷 [Français](README.fr.md) · 🇩🇪 [Deutsch](README.de.md) · 🇨🇳 [中文](README.zh.md)

## Funktionen
- Echte Aufzeichnung via ptrace.
- Zeitreise, Checkpoints, Zustands-Diff.
- Speicher-Explorer mit Hex und Strings.
- Ausführungsgraph und Aufrufstapel.
- Qt6-GUI und CLI.
- Binärformat v2 mit zstd-Kompression.

## Schnellstart
```bash
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## CLI-Befehle
trace, info, maps, checkpoints, memdiff, state, search, branch, export-html, export-json, strings, reconstruct, stats, project-create/add/info.

## Lizenz
MIT.