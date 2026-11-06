# CHRONOS

![CI](https://github.com/CRISTOP-bot/chronos/actions/workflows/ci.yml/badge.svg)

Temporal Execution & Reconstruction Engine — reverse debugging con capacidades de reconstrucción temporal.

Estado: Fase 1 (fundación) completada. Ver docs/00_RESEARCH.md.

## Compilar

```bash
cmake -S . -B build
cmake --build build
```

## Ejecutar tests

```bash
cd build && ctest
```

## Demo

```bash
./build/tools/chronos_cli demo /tmp/opencode/demo.chronos
./build/tools/chronos_cli trace /tmp/opencode/hello /tmp/opencode/hello.chronos
./build/tools/chronos_cli info /tmp/opencode/hello.chronos
./build/tools/chronos_cli checkpoints /tmp/opencode/hello.chronos
./build/tools/chronos_cli memdiff /tmp/opencode/hello.chronos 1 2
./build/tools/chronos_cli state /tmp/opencode/d2.chronos 3
./build/tools/chronos_cli search /tmp/opencode/hello.chronos write
./build/tools/chronos_cli maps $$
./build/tools/chronos_cli branch /tmp/opencode/hello.chronos /tmp/opencode/branch.chronos label
```
