#include "wandb_client/run.h"

#include "wandb_client/config.h"
#include "wandb_client/py_runtime.h"

#include <pybind11/stl.h>

namespace wandb {

namespace {

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
    return Run(std::move(run_obj));
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("wandb.init() failed: ") + e.what());
  }
}

// -- Instance methods --------------------------------------------------------

Run::Run(py::object run_obj) : run_(std::move(run_obj)), active_(true) {}

Run::~Run() {
  if (active_) {
    try {
      finish();
    } catch (...) {
      // Suppress exceptions in destructor.
    }
  }
}

Run::Run(Run &&other) noexcept
    : run_(std::move(other.run_)), active_(other.active_) {
  other.active_ = false;
}

Run &Run::operator=(Run &&other) noexcept {
  if (this != &other) {
    if (active_) {
      try {
        finish();
      } catch (...) {
      }
    }
    run_ = std::move(other.run_);
    active_ = other.active_;
    other.active_ = false;
  }
  return *this;
}

void Run::log(const std::map<std::string, double> &metrics) {
  GILGuard gil;

  try {
    py::dict py_metrics;
    for (const auto &[key, val] : metrics) {
      py_metrics[py::cast(key)] = py::cast(val);
    }
    run_.attr("log")(py_metrics);
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.log() failed: ") + e.what());
  }
}

void Run::set_summary(const std::string &key, double value) {
  GILGuard gil;

  try {
    run_.attr("summary")[py::cast(key)] = py::cast(value);
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.summary[] assignment failed: ") +
                         e.what());
  }
}

void Run::update_config(const std::map<std::string, ConfigValue> &updates) {
  GILGuard gil;

  try {
    py::dict py_updates;
    for (const auto &[key, val] : updates) {
      py_updates[py::cast(key)] = config_value_to_py(val);
    }
    run_.attr("config").attr("update")(py_updates);
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.config.update() failed: ") +
                         e.what());
  }
}

void Run::finish() {
  if (!active_) {
    return;
  }

  GILGuard gil;

  try {
    run_.attr("finish")();
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("run.finish() failed: ") + e.what());
  }

  active_ = false;
}

std::string Run::id() const {
  GILGuard gil;

  try {
    py::object attr = run_.attr("id");
    if (attr.is_none())
      return "";
    return attr.cast<std::string>();
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("Failed to read run.id: ") + e.what());
  }
}

std::string Run::name() const {
  GILGuard gil;

  try {
    py::object attr = run_.attr("name");
    if (attr.is_none())
      return "";
    return attr.cast<std::string>();
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("Failed to read run.name: ") + e.what());
  }
}

std::string Run::url() const {
  GILGuard gil;

  try {
    py::object attr = run_.attr("url");
    if (attr.is_none())
      return "";
    return attr.cast<std::string>();
  } catch (const py::error_already_set &e) {
    throw WandbException(std::string("Failed to read run.url: ") + e.what());
  }
}

} // namespace wandb
