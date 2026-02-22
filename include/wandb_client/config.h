#pragma once

#include <pybind11/embed.h>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace py = pybind11;

namespace wandb {

/// A config value that can be one of several scalar types.
/// Used in RunConfig::config to represent hyperparameters.
using ConfigValue = std::variant<int, double, bool, std::string>;

/// Configuration for initializing a wandb run.
///
/// Holds all parameters that map to `wandb.init()` keyword arguments.
/// Only `project` is required; all other fields are optional.
///
/// Usage:
///   RunConfig cfg;
///   cfg.project = "my-project";
///   cfg.entity = "my-team";
///   cfg.config["lr"] = 0.001;
///   auto run = Run::init(cfg);
struct RunConfig {
  /// W&B project name (required).
  std::string project;

  /// W&B entity (team or username). Empty = use default entity.
  std::string entity;

  /// Short display name for the run. Empty = auto-generated.
  std::string name;

  /// Longer description / notes for the run.
  std::string notes;

  /// Tags for filtering and grouping runs.
  std::vector<std::string> tags;

  /// Hyperparameters / config dict logged with the run.
  std::map<std::string, ConfigValue> config;

  /// Run mode: "online", "offline", or "disabled". Empty = use env default.
  std::string mode;

  /// Directory for local wandb run data. Empty = default (./wandb).
  std::string dir;
};

/// Converts a RunConfig to a Python dict of keyword arguments for wandb.init().
///
/// Only non-empty / non-default fields are included in the output dict.
/// Requires the GIL to be held by the caller.
py::dict config_to_py_dict(const RunConfig &cfg);

/// Converts a ConfigValue variant to a py::object.
///
/// Maps int→int, double→float, bool→bool, string→str in Python.
/// Requires the GIL to be held by the caller.
py::object config_value_to_py(const ConfigValue &value);

} // namespace wandb
