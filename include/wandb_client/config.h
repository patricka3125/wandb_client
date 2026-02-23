#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

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

  /// System metrics sampling interval in seconds.
  /// Controls how often wandb samples CPU/GPU/memory utilization.
  /// 0.0 = use wandb default (15s). Must be > 0 to take effect.
  double stats_sampling_interval = 0.0;
};

} // namespace wandb
