#include "wandb_client/metrics.h"
#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

#include <gtest/gtest.h>

#include <thread>

namespace wandb {
namespace {

// Integration test fixture for online metrics tests.
// Requires a valid WANDB_API_KEY in .env or the environment.
class MetricsOnlineTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    PyRuntime::instance().initialize();

    Run::login();

    // Verify the API key is available.
    GILGuard gil;
    py::object os = PyRuntime::instance().import("os");
    py::object key =
        os.attr("environ").attr("get")("WANDB_API_KEY", py::none());
    if (key.is_none() || key.cast<std::string>().empty()) {
      FAIL() << "WANDB_API_KEY not found. Set it in .env or the environment.";
    }
  }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

// Verifies that a run with custom system stats sampling interval
// and TrainingTimer works end-to-end against the live service.
TEST_F(MetricsOnlineTest, OnlineRunWithSystemMetrics) {
  RunConfig cfg;
  cfg.project = "wandb_client_test";
  cfg.mode = "online";
  cfg.stats_sampling_interval = 1.0;

  auto run = Run::init(cfg);
  EXPECT_FALSE(run.id().empty());

  // Simulate a training step with a timed section.
  {
    TrainingTimer timer(run, "train_step_ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  run.log({{"epoch", 1.0}, {"loss", 0.42}});

  {
    TrainingTimer timer(run, "eval_step_ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }

  run.log({{"epoch", 2.0}, {"loss", 0.35}});
  run.set_summary("best_loss", 0.35);

  run.finish();
}

} // namespace
} // namespace wandb
