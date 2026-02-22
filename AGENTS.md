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
├── tests/                  # GoogleTest unit tests (1:1 with src/)
│   └── integration/        # Integration tests (run against live wandb)
├── docs/                   # Design docs, roadmap, user guides
└── venv/                   # Python virtual environment (gitignored)
```

## Build System
- **CMake ≥ 3.20** with FetchContent for dependencies.
- pybind11 and GoogleTest are fetched automatically — no manual installs.
- Python 3 is discovered via `find_package(Python3)`.
- **Build & unit tests**: `./build.sh` (or `./build.sh --clean` for a clean rebuild)
- **Integration tests only**: `./build.sh --integration` (requires API key in `.env`)
- Flags can be combined: `./build.sh --clean --integration`

## Testing
- **Framework**: GoogleTest
- **1:1 mapping**: every `src/foo.cc` has `tests/test_foo.cc`.
- **Unit tests** (`tests/`): run in `WANDB_MODE=offline` — no network.
- **Integration tests** (`tests/integration/`): run against the live W&B service.
  Integration tests **must fail** (not skip) if the API key is missing.
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

## Secrets & Environment
- **`.env`** lives in the project root and is **gitignored**.
- Contains `WANDB_API_KEY` and potentially other secrets.
- **Never read, modify, log, or commit the `.env` file** unless given explicit permission.
- API key loading is handled automatically by `Run::login()` via `python-dotenv`.
