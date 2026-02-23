#pragma once

#include "wandb_client/config.h"
#include "wandb_client/py_runtime.h"

#include <gtest/gtest.h>

namespace wandb {
namespace testing {

/// Creates a minimal RunConfig suitable for offline unit testing.
/// Returns a config with project set and mode="offline".
inline RunConfig
make_offline_config(const std::string &project = "wandb_client_unit_test") {
  RunConfig cfg;
  cfg.project = project;
  cfg.mode = "offline";
  return cfg;
}

/// Test fixture base that initializes the Python runtime for the suite and
/// acquires/releases the GIL per-test. Inherit from this when tests directly
/// work with py::object (e.g. calling import(), config_to_py_dict()).
///
/// Usage:
///   class MyTest : public GILScopedFixture {};
///   TEST_F(MyTest, SomeTest) {
///       py::object mod = PyRuntime::instance().import("sys");
///       // GIL is held — no manual GILGuard needed.
///   }
class GILScopedFixture : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }

  /// Acquires the GIL before each test so tests can work with py::object
  /// without manual GILGuard.
  void SetUp() override { gil_state_ = PyGILState_Ensure(); }

  /// Releases the GIL after the test (and after all py::object locals are
  /// destroyed).
  void TearDown() override { PyGILState_Release(gil_state_); }

private:
  PyGILState_STATE gil_state_;
};

} // namespace testing
} // namespace wandb
