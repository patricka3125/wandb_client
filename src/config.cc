#include "wandb_client/config.h"

#include <pybind11/stl.h>

namespace wandb {

py::object config_value_to_py(const ConfigValue &value) {
  return std::visit([](auto &&v) -> py::object { return py::cast(v); }, value);
}

py::dict config_to_py_dict(const RunConfig &cfg) {
  py::dict kwargs;

  // Project is always set (required field).
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

  // Convert the config map to a nested Python dict.
  if (!cfg.config.empty()) {
    py::dict py_config;
    for (const auto &[key, val] : cfg.config) {
      py_config[py::cast(key)] = config_value_to_py(val);
    }
    kwargs["config"] = py_config;
  }

  // Build a wandb.Settings object if any settings fields are specified.
  if (cfg.stats_sampling_interval > 0.0) {
    py::object wandb_mod = py::module_::import("wandb");
    py::object settings = wandb_mod.attr("Settings")(
        py::arg("x_stats_sampling_interval") = cfg.stats_sampling_interval);
    kwargs["settings"] = settings;
  }

  return kwargs;
}

} // namespace wandb
