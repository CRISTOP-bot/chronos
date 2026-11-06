# Contributing to CHRONOS

Thanks for your interest in contributing! CHRONOS is an engineering tool; quality and honesty matter.

## Rules
- Do not simulate functionality. Implement real behavior or document limitations.
- Do not introduce unnecessary dependencies.
- Keep architecture modular (one subsystem per directory).
- Add tests for new features.
- Run `ctest` before submitting.

## Build
```bash
cmake -S . -B build
cmake --build build
cd build && ctest
```

## Pull Requests
- Use clear, atomic commits.
- Update docs if behavior changes.
- Fill the PR template.

## Code of Conduct
Be respectful and constructive. See CODE_OF_CONDUCT.md.