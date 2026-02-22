# WandB C++ Client Bridge

A C++ interface that bridges to the [Weights & Biases](https://wandb.ai) Python SDK via [pybind11](https://github.com/pybind/pybind11). Log metrics, manage artifacts, and use the model registry — all from C++.

## Prerequisites

| Tool | Minimum Version |
|------|----------------|
| CMake | 3.20 |
| C++ compiler | C++17 support (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 19.20) |
| Python | 3.13+ |
| pip | latest |
| GoogleTest | System install (via brew/apt) |

## Quick Start

### 1. Clone & Set Up the Python Virtual Environment

```bash
# Clone the repository
git clone <repo-url> wandb_client && cd wandb_client

# Create a Python virtual environment and install dependencies
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
deactivate
```

### 2. Build

```bash
cmake -B build -DPython3_EXECUTABLE=$(pwd)/venv/bin/python3
cmake --build build

# Symlink compile_commands.json for IDE support
ln -sf build/compile_commands.json compile_commands.json
```

### 3. Run Tests

```bash
cd build && ctest --output-on-failure
```

All tests run with `WANDB_MODE=offline` by default, so no W&B account is required.

## Project Structure

```
wandb_client/
├── CMakeLists.txt              # Top-level build config
├── AGENTS.md                   # Project conventions
├── README.md                   # This file
├── requirements.txt            # Python dependencies
├── docs/
│   ├── design.md               # Architecture & API design
│   └── roadmap.md              # Implementation roadmap
├── include/wandb_client/       # Public C++ headers
│   └── py_runtime.h            # Python interpreter manager
├── src/                        # Implementation
│   └── py_runtime.cc
├── tests/                      # GoogleTest tests
│   └── test_py_runtime.cc
└── venv/                       # Python venv (gitignored)
```

## Environment Variables

| Variable | Description |
|----------|-------------|
| `WANDB_MODE` | Set to `offline` to disable network calls |
| `WANDB_VENV_PATH` | Override the auto-detected venv path |
| `WANDB_API_KEY` | Your W&B API key (for online mode) |

## License

See [LICENSE](LICENSE).
