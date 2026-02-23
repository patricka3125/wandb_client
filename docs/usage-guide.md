# WandB C++ Client Bridge — User Guide

A C++ library that bridges to the [Weights & Biases](https://wandb.ai) Python SDK via [pybind11](https://github.com/pybind/pybind11), enabling experiment tracking directly from C++ code.

---

## Prerequisites

| Requirement | Minimum Version |
|---|---|
| CMake | 3.20 |
| C++ compiler | C++17 support |
| Python | 3.13+ |
| pip packages | `wandb`, `python-dotenv` |

> **Note:** pybind11 and GoogleTest are fetched automatically by CMake — no manual install needed.

---

## Quick Start

### 1. Clone & set up the virtual environment

```bash
git clone <repo-url> wandb_client
cd wandb_client
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
```

### 2. Configure your API key

Create a `.env` file in the project root (this file is gitignored):

```
WANDB_API_KEY=your_api_key_here
```

You can find your API key at [https://wandb.ai/authorize](https://wandb.ai/authorize).

### 3. Build

```bash
./build.sh          # Build + run unit tests
./build.sh --clean  # Clean rebuild + unit tests
```

### 4. Run integration tests (optional)

```bash
./build.sh --integration  # Runs tests against the live W&B service
```

---

## API Reference

All classes and functions live in the `wandb` namespace. Include headers from `wandb_client/`.

### PyRuntime

```cpp
#include "wandb_client/py_runtime.h"
```

Singleton managing the embedded Python interpreter lifecycle. Must be initialized before any wandb operations.

```cpp
wandb::PyRuntime::instance().initialize();
// ... use wandb API ...
wandb::PyRuntime::instance().finalize();
```

### RunConfig

```cpp
#include "wandb_client/config.h"
```

Configuration struct for `wandb.init()`. Fields map directly to `wandb.init()` keyword arguments.

| Field | Type | Required | Description |
|---|---|---|---|
| `project` | `std::string` | **Yes** | W&B project name |
| `entity` | `std::string` | No | Team or username |
| `name` | `std::string` | No | Display name for the run |
| `notes` | `std::string` | No | Run description |
| `tags` | `std::vector<std::string>` | No | Tags for filtering |
| `config` | `std::map<std::string, ConfigValue>` | No | Hyperparameters |
| `mode` | `std::string` | No | `"online"`, `"offline"`, or `"disabled"` |
| `dir` | `std::string` | No | Local data directory |
| `stats_sampling_interval` | `double` | No | Native system metrics (CPU/GPU) sampling rate in seconds. 0.0 = use wandb default (15s). |

`ConfigValue` is a `std::variant<int, double, bool, std::string>`.

### Run

```cpp
#include "wandb_client/run.h"
```

Main interface for experiment tracking.

| Method | Signature | Description |
|---|---|---|
| `login` | `static void login(const std::string& api_key = "")` | Loads `.env`, authenticates with W&B |
| `init` | `static Run init(const RunConfig& cfg)` | Creates a new run |
| `log` | `void log(const std::map<std::string, double>& metrics)` | Log scalar metrics |
| `set_summary` | `void set_summary(const std::string& key, double value)` | Set a summary metric |
| `update_config` | `void update_config(const std::map<std::string, ConfigValue>&)` | Update run config |
| `finish` | `void finish()` | End the run (also called by destructor) |
| `id` | `std::string id() const` | Unique run ID |
| `name` | `std::string name() const` | Display name (empty in offline mode until synced) |
| `url` | `std::string url() const` | Dashboard URL (empty in offline mode until synced) |

### Metrics & System Monitoring

WandB automatically collects basic system metrics like CPU percent, memory usage, disk IO, and GPU utilization. This native telemetry runs automatically in the background. You can adjust how often it samples hardware statistics by setting `cfg.stats_sampling_interval`.

If you need to measure latencies of specific sections of your C++ code (for instance, the duration of an epoch or forward pass), you can use `TrainingTimer`.

```cpp
#include "wandb_client/metrics.h"
```

A RAII scoped timer that measures wall-clock elapsed time:

| Method | Signature | Description |
|---|---|---|
| Constructor | `TrainingTimer(Run&, const std::string& label)` | Starts timer, `label` is the metric name |
| Destructor | `~TrainingTimer()` | Computes elapsed time (ms) and logs it to `run.log()` |
| `stop` | `void stop()` | Manually stop the timer and log immediately. Destructor becomes a no-op. |
| `elapsed_ms` | `double elapsed_ms() const` | Current elapsed time |

---

## Usage Example

```cpp
#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

int main() {
    // 1. Start the Python interpreter
    wandb::PyRuntime::instance().initialize();

    // 2. Authenticate (loads API key from .env)
    wandb::Run::login();

    // 3. Configure the run
    wandb::RunConfig cfg;
    cfg.project = "my-ml-project";
    cfg.name = "experiment-1";
    // Increase system monitoring frequency from 15s to 2s
    cfg.stats_sampling_interval = 2.0;
    cfg.config["learning_rate"] = 0.001;
    cfg.config["batch_size"] = 32;
    cfg.config["epochs"] = 100;

    // 4. Initialize
    auto run = wandb::Run::init(cfg);

    // 5. Training loop
    for (int epoch = 0; epoch < 100; ++epoch) {
        // Measure epoch latency using the TrainingTimer
        wandb::TrainingTimer epoch_timer(run, "epoch_compute_ms");

        double loss = 1.0 / (epoch + 1);  // placeholder
        double acc = 1.0 - loss;

        run.log({{"loss", loss}, {"accuracy", acc}});
    } // epoch_timer goes out of scope and logs epoch latency

    // 6. Record final metrics
    run.set_summary("best_accuracy", 0.99);

    // 7. Finish (or let RAII handle it)
    run.finish();

    // 8. Shut down
    wandb::PyRuntime::instance().finalize();
    return 0;
}
```

---

## Integrating into an Existing C++ Project

### Option A: Add as a subdirectory (recommended)

If your project uses CMake, add wandb_client as a subdirectory:

```cmake
# In your top-level CMakeLists.txt
add_subdirectory(path/to/wandb_client)

# Link your target
target_link_libraries(your_target PRIVATE wandb_client)
```

### Option B: Install and find_package

```bash
cd wandb_client
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install
```

Then in your CMakeLists.txt:

```cmake
find_package(wandb_client REQUIRED)
target_link_libraries(your_target PRIVATE wandb_client)
```

### Requirements for integration

1. **Python venv**: The wandb_client library expects a Python virtual environment with `wandb` and `python-dotenv` installed. Set the `WANDB_VENV_PATH` environment variable to point to your venv if it is not located at `./venv` relative to your working directory.

2. **API key**: Place a `.env` file with `WANDB_API_KEY=...` in the working directory of your application, or set the `WANDB_API_KEY` environment variable directly.

3. **Lifecycle**: Call `PyRuntime::instance().initialize()` once at program start and `PyRuntime::instance().finalize()` once at shutdown. The interpreter is a singleton — safe to use from anywhere after initialization.

4. **Thread safety**: Always acquire the GIL before Python calls. The `Run` class methods handle this internally. If making direct Python calls, use `wandb::GILGuard`.

---

## Error Handling

All wandb errors are thrown as `wandb::WandbException` (derives from `std::runtime_error`):

```cpp
try {
    auto run = wandb::Run::init(cfg);
} catch (const wandb::WandbException& e) {
    std::cerr << "W&B error: " << e.what() << std::endl;
}
```

---

## Offline Mode

For testing or environments without network access, set `mode` to `"offline"`:

```cpp
wandb::RunConfig cfg;
cfg.project = "my-project";
cfg.mode = "offline";

auto run = wandb::Run::init(cfg);
run.log({{"loss", 0.5}});
run.finish();
```

Offline runs are saved locally and can be synced later with:

```bash
wandb sync path/to/offline-run-directory
```
