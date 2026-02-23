#include "wandb_client/run.h"

#include "wandb_client/config.h"
#include "wandb_client/py_runtime.h"

#include <pybind11/stl.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

namespace wandb {

/// Internal state backing a Run object.
///
/// Owns the Python run object, a background worker thread, and the async log
/// queue. All members are private; Run interacts through public methods.
struct RunState {
  /// Constructs the state and spawns the background log worker.
  explicit RunState(py::object r) : run_(std::move(r)), active_(true) {
    worker_ = std::thread([this]() { worker_loop(); });
  }

  /// Destructor: drains the queue, finishes the run, and releases the Python
  /// object under the GIL.
  ~RunState() {
    shutdown(false);
    GILGuard gil;
    run_ = py::object();
  }

  // Non-copyable, non-movable.
  RunState(const RunState &) = delete;
  RunState &operator=(const RunState &) = delete;

  /// Enqueues a metrics map for asynchronous logging.
  /// Throws WandbException if the run has already been finished.
  void enqueue(const std::map<std::string, double> &metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stop_worker_) {
      throw WandbException("Cannot log to finished or finishing run.");
    }
    log_queue_.push(metrics);
    cv_.notify_one();
  }

  /// Returns true if the run is still active (not yet finished).
  bool is_active() const { return active_.load(std::memory_order_acquire); }

  /// Signals the worker to drain and stop, calls run.finish() under the GIL.
  /// Uses atomic exchange to guarantee exactly-once execution.
  void shutdown(bool throw_on_error) {
    // Atomically test-and-set: only the first caller proceeds.
    bool expected = true;
    if (!active_.compare_exchange_strong(expected, false,
                                         std::memory_order_acq_rel)) {
      return;
    }

    // Signal the worker to drain remaining items and exit.
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_worker_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }

    GILGuard gil;
    try {
      run_.attr("finish")();
    } catch (const py::error_already_set &e) {
      if (throw_on_error) {
        throw WandbException(std::string("run.finish() failed: ") + e.what());
      } else {
        std::cerr << "[wandb_client] run.finish() failed: " << e.what() << "\n";
      }
    }
  }

  /// Returns the number of log calls that failed in the background worker.
  uint64_t dropped_log_count() const {
    return dropped_logs_.load(std::memory_order_relaxed);
  }

  /// Provides direct access to the underlying py::object for read operations
  /// (e.g., run.id, run.name, run.url, run.summary). Caller must hold the GIL.
  py::object &py_run() { return run_; }

private:
  /// Background worker loop: dequeues metrics and calls Python run.log().
  /// Drains all remaining items after stop is signaled before exiting.
  void worker_loop() {
    while (true) {
      std::map<std::string, double> metrics;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_worker_ || !log_queue_.empty(); });
        if (log_queue_.empty()) {
          // stop_worker_ is true and queue is empty — exit.
          break;
        }
        metrics = std::move(log_queue_.front());
        log_queue_.pop();
      }

      GILGuard gil;
      try {
        py::dict py_metrics;
        for (const auto &[key, val] : metrics) {
          py_metrics[py::cast(key)] = py::cast(val);
        }
        run_.attr("log")(py_metrics);
      } catch (const py::error_already_set &e) {
        dropped_logs_.fetch_add(1, std::memory_order_relaxed);
        std::cerr << "[wandb_client bg] run.log() failed: " << e.what() << "\n";
      }
    }
  }

  py::object run_;
  std::atomic<bool> active_{false};
  std::atomic<bool> stop_worker_{false};
  std::atomic<uint64_t> dropped_logs_{0};
  std::queue<std::map<std::string, double>> log_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread worker_;
};

namespace {

/// Converts a ConfigValue variant to a py::object.
/// Caller must hold the GIL.
py::object config_value_to_py(const ConfigValue &value) {
  return std::visit([](auto &&v) -> py::object { return py::cast(v); }, value);
}

/// Converts a RunConfig to a Python dict of keyword arguments for wandb.init().
/// Only non-empty / non-default fields are included. Caller must hold the GIL.
py::dict config_to_py_dict(const RunConfig &cfg) {
  py::dict kwargs;

  kwargs["project"] = py::cast(cfg.project);

  if (!cfg.entity.empty()) {
    kwargs["entity"] = py::cast(cfg.entity);
  }
  if (!cfg.name.empty()) {
    kwargs["name"] = py::cast(cfg.name);
  }
  if (!cfg.notes.empty()) {
    kwargs["notes"] = py::cast(cfg.notes);
  }
  if (!cfg.tags.empty()) {
    kwargs["tags"] = py::cast(cfg.tags);
  }
  if (!cfg.mode.empty()) {
    kwargs["mode"] = py::cast(cfg.mode);
  }
  if (!cfg.dir.empty()) {
    kwargs["dir"] = py::cast(cfg.dir);
  }

  if (!cfg.config.empty()) {
    py::dict py_config;
    for (const auto &[key, val] : cfg.config) {
      py_config[py::cast(key)] = config_value_to_py(val);
    }
    kwargs["config"] = py_config;
  }

  if (cfg.stats_sampling_interval > 0.0) {
    py::object wandb_mod = py::module_::import("wandb");
    py::object settings = wandb_mod.attr("Settings")(
        py::arg("x_stats_sampling_interval") = cfg.stats_sampling_interval);
    kwargs["settings"] = settings;
  }

  return kwargs;
}

/// Loads environment variables from the project .env file via python-dotenv.
/// Searches upward from the current working directory.
void load_dotenv() {
  try {
    py::object dotenv = PyRuntime::instance().import("dotenv");
    dotenv.attr("load_dotenv")();
  } catch (const py::error_already_set &e) {
    throw WandbException(
        std::string("Failed to load .env via python-dotenv: ") + e.what());
  }
}

} // namespace

// -- Static methods ----------------------------------------------------------

void Run::login(const std::string &api_key) {
  GILGuard gil;

  load_dotenv();

  try {
    py::object wandb_mod = PyRuntime::instance().import("wandb");
    if (api_key.empty()) {
      wandb_mod.attr("login")();
    } else {
      wandb_mod.attr("login")(py::arg("key") = api_key);
    }
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("wandb.login() failed: ") + e.what());
  }
}

Run Run::init(const RunConfig &cfg) {
  GILGuard gil;

  try {
    py::object wandb_mod = PyRuntime::instance().import("wandb");
    py::dict kwargs = config_to_py_dict(cfg);
    py::object run_obj = wandb_mod.attr("init")(**kwargs);
    return Run(std::make_unique<RunState>(std::move(run_obj)));
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("wandb.init() failed: ") + e.what());
  }
}

// -- Instance methods --------------------------------------------------------

Run::Run(std::unique_ptr<RunState> state) : state_(std::move(state)) {}

Run::~Run() = default;

Run::Run(Run &&other) noexcept : state_(std::move(other.state_)) {}

Run &Run::operator=(Run &&other) noexcept {
  if (this != &other) {
    state_ = std::move(other.state_);
  }
  return *this;
}

void Run::log(const std::map<std::string, double> &metrics) {
  if (!state_) {
    throw WandbException("Cannot log to invalid run.");
  }
  state_->enqueue(metrics);
}

void Run::set_summary(const std::string &key, double value) {
  if (!state_ || !state_->is_active())
    return;
  GILGuard gil;

  try {
    state_->py_run().attr("summary")[py::cast(key)] = py::cast(value);
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.summary[] assignment failed: ") +
                         e.what());
  }
}

void Run::update_config(const std::map<std::string, ConfigValue> &updates) {
  if (!state_ || !state_->is_active())
    return;
  GILGuard gil;

  try {
    py::dict py_updates;
    for (const auto &[key, val] : updates) {
      py_updates[py::cast(key)] = config_value_to_py(val);
    }
    state_->py_run().attr("config").attr("update")(py_updates);
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.config.update() failed: ") +
                         e.what());
  }
}

void Run::finish() {
  if (state_) {
    state_->shutdown(true /* throw_on_error */);
  }
}

/// Reads a string attribute from the Python run object.
/// Returns "" if the run is inactive or the attribute is None.
static std::string read_run_string_attr(RunState *state,
                                        const char *attr_name) {
  if (!state || !state->is_active())
    return "";
  GILGuard gil;

  try {
    py::object attr = state->py_run().attr(attr_name);
    if (attr.is_none())
      return "";
    return attr.cast<std::string>();
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("Failed to read run.") + attr_name + ": " +
                         e.what());
  }
}

std::string Run::id() const { return read_run_string_attr(state_.get(), "id"); }

std::string Run::name() const {
  return read_run_string_attr(state_.get(), "name");
}

std::string Run::url() const {
  return read_run_string_attr(state_.get(), "url");
}

} // namespace wandb
