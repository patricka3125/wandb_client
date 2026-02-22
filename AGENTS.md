# WandB C++ Client Bridge — Project Conventions

## Language & Standard
- **C++17** minimum. Use `std::string`, `std::map`, `std::optional`, etc.
- Prefer RAII for resource management (GIL, file handles, timers).

## Naming Conventions
- **Files**: `snake_case.cc`, `snake_case.h`
- **Classes**: `PascalCase` (e.g., `PyRuntime`, `GILGuard`, `RunConfig`)
- **Functions / Methods**: `snake_case()` for public API, `PascalCase()` for internal helpers
- **Constants / Macros**: `UPPER_SNAKE_CASE`
- **Namespaces**: `wandb` (top-level), `wandb::metrics` (sub-namespace)

## Directory Layout
```
wandb_client/
├── include/wandb_client/   # Public headers
├── src/                    # Implementation files
├── tests/                  # GoogleTest test files (1:1 with src/)
├── docs/                   # Design docs, roadmap
└── venv/                   # Python virtual environment (gitignored)
```

## Build System
- **CMake ≥ 3.20** with FetchContent for dependencies.
- pybind11 and GoogleTest are fetched automatically — no manual installs.
- Python 3 is discovered via `find_package(Python3)`.
- Build: `cmake -B build && cmake --build build`
- Test:  `cd build && ctest --output-on-failure`

## Testing
- **Framework**: GoogleTest
- **1:1 mapping**: every `src/foo.cc` has `tests/test_foo.cc`.
- **Offline mode**: set `WANDB_MODE=offline` in tests (no network).
- **Coverage target**: >60%.
- Helper methods should be defined outside the test function body.

## Python Integration
- All Python calls go through pybind11 under the GIL.
- `PyRuntime::instance()` is the singleton that owns the interpreter lifecycle.
- Use `GILGuard` (RAII) to acquire the GIL before any Python call.
- Python dependencies live in `wandb_client/venv/`.
- The venv path is set via `PYTHONHOME` or `sys.path` manipulation at interpreter init.

## Error Handling
- Python exceptions are caught at the bridge boundary via `py::error_already_set`.
- Rethrow as `WandbException` (derives from `std::runtime_error`).

## Comments & Documentation
- Inline comments within functions should be short.
- Every function/method declaration must have a comment above describing its
  functionality and intended usage.
- Public headers should include Doxygen-style `///` doc comments.

## Code Quality
- No working files, text/md, or tests saved to the root folder (use `docs/`, `tests/`, etc.).
- Helper methods needed for a function should be defined outside the caller.
