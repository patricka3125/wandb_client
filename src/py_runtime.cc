#include "wandb_client/py_runtime.h"

#include <pybind11/embed.h>

#include <filesystem>
#include <sstream>
#include <string>

namespace wandb {

namespace {

/// Locates the project-local venv site-packages directory.
/// Returns an empty string if the venv is not found.
std::string find_venv_site_packages() {
  // Walk upward from the executable to find the venv.
  // The venv is expected at <project_root>/venv/.
  // We also accept WANDB_VENV_PATH env var as an override.
  const char *env_path = std::getenv("WANDB_VENV_PATH");
  std::filesystem::path venv_root;

  if (env_path && std::strlen(env_path) > 0) {
    venv_root = std::filesystem::path(env_path);
  } else {
    // Default: look relative to CMake source dir (set at build time)
    // or fall back to a path relative to the current working directory.
    auto cwd = std::filesystem::current_path();
    // Try common locations
    for (auto candidate :
         {cwd / "venv", cwd / ".." / "venv", cwd / ".." / ".." / "venv"}) {
      if (std::filesystem::exists(candidate / "bin" / "python3") ||
          std::filesystem::exists(candidate / "bin" / "python")) {
        venv_root = std::filesystem::canonical(candidate);
        break;
      }
    }
  }

  if (venv_root.empty()) {
    return "";
  }

  // Find site-packages under lib/python3.X/site-packages
  auto lib_dir = venv_root / "lib";
  if (!std::filesystem::exists(lib_dir)) {
    return "";
  }

  for (auto &entry : std::filesystem::directory_iterator(lib_dir)) {
    if (entry.is_directory()) {
      auto name = entry.path().filename().string();
      if (name.rfind("python", 0) == 0) {
        auto sp = entry.path() / "site-packages";
        if (std::filesystem::exists(sp)) {
          return std::filesystem::canonical(sp).string();
        }
      }
    }
  }

  return "";
}

} // namespace

// -- PyRuntime ---------------------------------------------------------------

PyRuntime &PyRuntime::instance() {
  static PyRuntime runtime;
  return runtime;
}

PyRuntime::~PyRuntime() {
  if (initialized_) {
    finalize();
  }
}

void PyRuntime::initialize() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialized_) {
    return;
  }

  try {
    py::initialize_interpreter();
  } catch (const std::exception &e) {
    throw WandbException(
        std::string("Failed to initialize Python interpreter: ") + e.what());
  }

  initialized_ = true;

  // Inject venv site-packages into sys.path so `import wandb` works.
  try {
    std::string site_packages = find_venv_site_packages();
    if (!site_packages.empty()) {
      py::module_::import("sys").attr("path").attr("insert")(0, site_packages);
    }
  } catch (const py::error_already_set &e) {
    // Non-fatal: venv might not exist yet, or sys.path mutation failed.
    // The user can still set PYTHONPATH manually.
  }
}

void PyRuntime::finalize() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!initialized_) {
    return;
  }

  module_cache_.clear();
  py::finalize_interpreter();
  initialized_ = false;
}

bool PyRuntime::is_initialized() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return initialized_;
}

py::object PyRuntime::import(const std::string &module) {
  // Caller must hold the GIL.
  auto it = module_cache_.find(module);
  if (it != module_cache_.end()) {
    return it->second;
  }

  try {
    py::object mod = py::module_::import(module.c_str());
    module_cache_[module] = mod;
    return mod;
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("Failed to import Python module '") +
                         module + "': " + e.what());
  }
}

// -- GILGuard ----------------------------------------------------------------

GILGuard::GILGuard() : state_(PyGILState_Ensure()) {}

GILGuard::~GILGuard() { PyGILState_Release(state_); }

} // namespace wandb
