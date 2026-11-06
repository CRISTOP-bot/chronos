# CHRONOS

![CI](https://img.shields.io/github/actions/workflow/status/CRISTOP-bot/chronos/ci.yml?style=for-the-badge&label=BUILD)
![License](https://img.shields.io/badge/LICENSE-MIT-00b4d8?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/PLATFORM-Linux%20x86__64-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**时间执行与重建引擎** — 真正的逆向调试与时间旅行分析工具。

CHRONOS 记录、重建并探索程序随时间的执行。它不是调试器克隆：它是理解程序内部发生了什么并恢复先前状态的工程工具。

🌐 **语言：** 🇬🇧 [English](README.en.md) · 🇪🇸 [Español](README.es.md) · 🇫🇷 [Français](README.fr.md) · 🇩🇪 [Deutsch](README.de.md) · 🇨🇳 [中文](README.zh.md)

## 功能
- 通过 ptrace 进行真实录制。
- 时间旅行、检查点、状态差异。
- 带十六进制和字符串的内存浏览器。
- 执行图和调用栈。
- Qt6 图形界面与 CLI。
- 带 zstd 压缩的二进制格式 v2。

## 快速开始
```bash
cmake -S . -B build -DCHRONOS_BUILD_GUI=ON
cmake --build build
cd build && ctest
```

## CLI 命令
trace, info, maps, checkpoints, memdiff, state, search, branch, export-html, export-json, strings, reconstruct, stats, project-create/add/info.

## 许可证
MIT.