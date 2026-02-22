#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

#include <gtest/gtest.h>

namespace wandb {
namespace {

// Test fixture that initializes the Python runtime for all run tests.
// WANDB_MODE=offline is set by CMake so these tests never hit the network.
class RunTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

// Helper: creates a minimal RunConfig suitable for offline testing.
RunConfig make_offline_config() {
  RunConfig cfg;
  cfg.project = "wandb_client_unit_test";
  cfg.mode = "offline";
  return cfg;
}

// Verifies that a run can be initialized and finished in offline mode.
TEST_F(RunTest, RunInitAndFinish) {
  auto run = Run::init(make_offline_config());
  EXPECT_FALSE(run.id().empty());
  // name() may return empty in offline mode (Python returns None).
  EXPECT_NO_THROW(run.name());
  run.finish();
}

// Verifies that logging scalar metrics does not throw.
TEST_F(RunTest, RunLogMetrics) {
  auto run = Run::init(make_offline_config());
  EXPECT_NO_THROW(run.log({{"loss", 0.5}, {"accuracy", 0.9}}));
  run.finish();
}

// Verifies that setting a summary metric does not throw.
TEST_F(RunTest, RunSetSummary) {
  auto run = Run::init(make_offline_config());
  EXPECT_NO_THROW(run.set_summary("best_loss", 0.1));
  run.finish();
}

// Verifies that updating config after init does not throw.
TEST_F(RunTest, RunUpdateConfig) {
  RunConfig cfg = make_offline_config();
  cfg.config["initial_lr"] = 0.01;
  auto run = Run::init(cfg);
  EXPECT_NO_THROW(run.update_config({{"added_key", std::string("value")}}));
  run.finish();
}

// Verifies that calling finish() twice is a safe no-op.
TEST_F(RunTest, RunDoubleFinishIsNoop) {
  auto run = Run::init(make_offline_config());
  run.finish();
  EXPECT_NO_THROW(run.finish());
}

// Verifies that the destructor calls finish() automatically (RAII).
TEST_F(RunTest, RunRAIIFinish) {
  EXPECT_NO_THROW({
    auto run = Run::init(make_offline_config());
    run.log({{"step", 1.0}});
    // run goes out of scope — destructor should call finish()
  });
}

} // namespace
} // namespace wandb
