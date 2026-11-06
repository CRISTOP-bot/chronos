# CHRONOS — Investigación Técnica y Plan

## 1. Arquitectura propuesta

CHRONOS se organiza en subsistemas independientes compilados como una biblioteca estática `chronos_core` (C++20), con frontends CLI/GUI encima:

- `core/common`: tipos fundamentales, errores, resultados.
- `core/events`: modelo de eventos y log de eventos indexado.
- `core/recording`: sesiones y motor de grabación.
- `core/execution`: administración de sesiones/procesos.
- `core/memory`: regiones de memoria y mapas.
- `core/timeline`: navegación temporal, checkpoints, búsqueda.
- `core/analysis`: snapshots y diffs de estado.
- `core/storage`: formato binario propio, lector/escritor.
- `instrumentation`: (Fase 2+) captura real mediante ptrace/LLVM.
- `debugger`: (Fase 3+) control de procesos.
- `gui`: (Fase 6+) Qt6.
- `tools`: CLI.
- `tests`, `benchmarks`.

Principios: modularidad, sin funcionalidades simuladas, integridad de grabaciones, almacenamiento por bloques/indexado, límites documentados.

## 2. Análisis de tecnologías

Disponibles en el entorno (CachyOS x86_64):
- Compilador: GCC 16.2 / Clang 23.1 (C++20 ok).
- CMake 4.4, Make, GDB 18.1.
- LLVM 23 (libLLVM, libclang, headers) para instrumentación.
- libelf + libdw (elfutils) para DWARF.
- zlib, zstd para compresión.
- Qt6 (Widgets/OpenGL) para GUI futura.
- GLFW/SDL2/OpenGL para visualización alternativa.
- GoogleTest 1.18 para pruebas.

No disponible: `rr` (mozilla rr). Se evaluará implementar capacidades equivalentes parcialmente.

## 3. Comparación con GDB y rr

- **GDB**: depurador interactivo, no graba ejecución completa; soporta reverse debugging solo si el target soporta (process record) con alto overhead y limitaciones. Útil como referencia de interfaz MI y lectura de DWARF.
- **rr**: record & replay determinista para Linux x86_64, graba syscalls y eventos, permite time-travel. Es la referencia directa. CHRONOS no puede igualar su determinismo inicialmente; adopta su modelo (grabación de eventos + checkpoints + replay) pero reconoce limitaciones (nondeterminismo de hardware/threads, syscalls, tiempo).
- CHRONOS aporta: formato propio indexado, análisis de estados, diff, branches analíticas, GUI técnica.

## 4. Limitaciones conocidas

- Sin instrumentación real aún (Fase 2). Actualmente sólo registro manual de eventos.
- Sin replay determinista real; reconstrucción se basa en snapshots/eventos, no en re-ejecución bit-a-bit.
- Sin GUI aún.
- Formato binario v1 sin compresión ni bloques incrementales aún (planificado).
- Soporte inicial: Linux x86_64, programas C simples.
- Hilos: modelo de eventos por hilo existe, pero sincronización avanzada pendiente.
- Memoria: seguimiento real de asignaciones/liberaciones pendiente (ptrace + hooks).
- Seguridad: grabaciones son datos pasivos; no se ejecutan.

## 5. Estructura definitiva

Ver árbol en sección 10 del prompt. Implementado en `/home/cristopher/chronos`.

## 6. Plan de implementación

Fases 0-7 según prompt. Estado actual:
- Fase 0 (investigación) completada.
- Fase 1 (fundación) completada.
- Fase 2 (Recording Engine) en progreso: captura real mediante ptrace (syscalls), memoria vía /proc, compresión zstd.
- Fases 3-7 pendientes (checkpoints avanzados, timeline optimizado, análisis, GUI, benchmarks).

Próximo: Fase 3 checkpoints con snapshots de memoria y reconstrucción validada.

## 7. Riesgos técnicos

- Determinismo y replay: alto riesgo; se documentará limitación.
- Overhead de instrumentación: impacto en rendimiento.
- Tamaño de grabaciones: mitigar con compresión/bloques/índices.
- DWARF parsing complejidad.
- Seguridad de procesos instrumentados (ptrace).
- Compatibilidad de versiones del formato.

## 8. Criterios de aceptación por fase

Fase 1 (actual):
- Proyecto compila con CMake.
- Tipos fundamentales y sistema de eventos.
- Pruebas unitarias pasan.
- Almacenamiento binario round-trip con integridad CRC.
- CLI demo guarda y reabre grabación.

Fase 2:
- Registrar eventos reales de un programa C.
- Sesiones persistentes.

Fase 3:
- Checkpoints y reconstrucción de estados previos validados por pruebas.

Fase 4:
- Navegación temporal e índices eficientes.

Fase 5:
- State Diff, Execution Graph, Memory Explorer básicos.

Fase 6:
- GUI funcional conectada a motores.

Fase 7:
- Benchmarks y pruebas de estrés/integridad.
