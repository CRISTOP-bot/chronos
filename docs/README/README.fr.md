# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**Moteur d'Exécution Temporelle et de Reconstruction** — outil réel de reverse debugging et d'analyse temporelle.

CHRONOS enregistre, reconstruit et explore les exécutions de programmes dans le temps. Ce n'est pas un clone de debugger : c'est un outil d'ingénierie pour comprendre ce qui s'est passé et récupérer des états antérieurs.

🌐 **Langues :** 🇬🇧 [English](README.en.md) · 🇪🇸 [Español](README.es.md) · 🇫🇷 [Français](README.fr.md) · 🇩🇪 [Deutsch](README.de.md) · 🇨🇳 [中文](README.zh.md)

## Fonctionnalités
- Enregistrement réel via ptrace.
- Navigation temporelle, checkpoints, diff d'états.
- Explorateur de mémoire avec hex et strings.
- Graphe d'exécution et pile d'appels.
- GUI Qt6 et CLI.
- Format binaire v2 avec compression zstd.

## Démarrage rapide
```bash
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## Commandes CLI
trace, info, maps, checkpoints, memdiff, state, search, branch, export-html, export-json, strings, reconstruct, stats, project-create/add/info.

## Licence
MIT.