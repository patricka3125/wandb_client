#pragma once

#include "wandb_client/config.h"

#include <pybind11/embed.h>

#include <map>
#include <string>

namespace py = pybind11;

namespace wandb {

/// C++ wrapper around a wandb Python Run object.
///
/// Provides methods to log metrics, set summary values, update config,
/// and finish the run. The Run object owns a reference to the underlying
/// Python `wandb.Run` instance and calls into it via pybind11 under the GIL.
///
/// Usage:
///   Run::login();  // loads .env, authenticates
///   auto run = Run::init(cfg);
///   run.log({{"loss", 0.5}});
///   run.finish();
class Run {
public:
  /// Authenticates with the wandb service.
  ///
  /// Loads environment variables from the project `.env` file using
  /// python-dotenv, then calls `wandb.login()`. If `api_key` is non-empty,
  /// it is passed directly; otherwise the key is read from `WANDB_API_KEY`
  /// in the environment or `.env` file.
  static void login(const std::string &api_key = "");

  /// Initializes a new wandb run with the given configuration.
  ///
  /// Calls `wandb.init(**kwargs)` where kwargs are built from `cfg`.
  /// Returns a Run object wrapping the Python run.
  static Run init(const RunConfig &cfg);

  /// Logs a dictionary of scalar metrics for the current step.
  ///
  /// Calls `run.log({"key": value, ...})` on the underlying Python run.
  void log(const std::map<std::string, double> &metrics);

  /// Sets a summary metric value.
  ///
  /// Calls `run.summary["key"] = value` on the underlying Python run.
  void set_summary(const std::string &key, double value);

  /// Updates the run config with additional key-value pairs.
  ///
  /// Calls `run.config.update({...})` on the underlying Python run.
  void update_config(const std::map<std::string, ConfigValue> &updates);

  /// Finishes the run, uploading any remaining data.
  ///
  /// Safe to call multiple times — subsequent calls are no-ops.
  void finish();

  /// Returns the unique run identifier (e.g. "abc12345").
  std::string id() const;

  /// Returns the human-readable run name (e.g. "cosmic-rain-42").
  std::string name() const;

  /// Returns the URL to the run's page on the wandb dashboard.
  std::string url() const;

  /// Destructor finishes the run if still active (RAII).
  ~Run();

  // Move-only: no copying allowed.
  Run(Run &&other) noexcept;
  Run &operator=(Run &&other) noexcept;
  Run(const Run &) = delete;
  Run &operator=(const Run &) = delete;

private:
  /// Private constructor — use Run::init() to create instances.
  explicit Run(py::object run_obj);

  py::object run_;
  bool active_ = false;
};

} // namespace wandb
