# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**Motor de Ejecución Temporal y Reconstrucción** — herramienta real de reverse debugging y análisis temporal de programas.

CHRONOS graba, reconstruye y explora ejecuciones de programas a lo largo del tiempo. No es un clon de debugger: es una herramienta de ingeniería para entender qué ocurrió dentro de un programa y recuperar estados anteriores.

🌐 **Idiomas:** 🇬🇧 [English](README.en.md) · 🇪🇸 [Español](README.es.md) · 🇫🇷 [Français](README.fr.md) · 🇩🇪 [Deutsch](README.de.md) · 🇨🇳 [中文](README.zh.md)

## Características
- Grabación real con ptrace.
- Navegación temporal, checkpoints, diff de estados.
- Explorador de memoria con hex y strings.
- Grafo de ejecución y pila de llamadas.
- GUI Qt6 y CLI.
- Formato binario v2 con compresión zstd.

## Inicio rápido
```bash
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## Comandos CLI
trace, info, maps, checkpoints, memdiff, state, search, branch, export-html, export-json, strings, reconstruct, stats, project-create/add/info.

## Licencia
MIT.