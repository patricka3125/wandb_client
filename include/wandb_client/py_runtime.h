#pragma once

#include <pybind11/embed.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace py = pybind11;

namespace wandb {

/// Base exception for all wandb bridge errors.
class WandbException : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

/// Singleton that owns the embedded CPython interpreter lifecycle.
///
/// Usage:
///   PyRuntime::instance().initialize();
///   // ... use pybind11 ...
///   PyRuntime::instance().finalize();
///
/// The interpreter is configured to use the project-local venv so that
/// `import wandb` resolves to the venv-installed package.
class PyRuntime {
public:
  /// Returns the singleton instance.
  static PyRuntime &instance();

  /// Starts the embedded Python interpreter and configures the venv sys.path.
  /// Throws WandbException if the interpreter cannot start.
  /// Safe to call multiple times — subsequent calls are no-ops.
  void initialize();

  /// Shuts down the embedded Python interpreter.
  /// Safe to call if not initialized (no-op).
  void finalize();

  /// Returns true if the interpreter is currently running.
  bool is_initialized() const;

  /// Imports a Python module by name, with caching. Requires GIL.
  /// The caller must hold the GIL, as the returned py::object's lifetime
  /// requires it (dec_ref on destruction).
  /// Throws WandbException if the import fails.
  py::object import(const std::string &module);

private:
  PyRuntime() = default;
  ~PyRuntime();

  PyRuntime(const PyRuntime &) = delete;
  PyRuntime &operator=(const PyRuntime &) = delete;

  bool initialized_ = false;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, py::object> module_cache_;
  std::unique_ptr<py::gil_scoped_release> gil_release_;
};

/// RAII guard that acquires the Python GIL on construction and releases it
/// on destruction.
///
/// Usage:
///   {
///       GILGuard gil;
///       // ... safe to call Python APIs ...
///   }
///   // GIL released here
class GILGuard {
public:
  GILGuard();
  ~GILGuard();

private:
  GILGuard(const GILGuard &) = delete;
  GILGuard &operator=(const GILGuard &) = delete;

  PyGILState_STATE state_;
};

} // namespace wandb
