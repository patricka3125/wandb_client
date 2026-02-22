#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

#include <gtest/gtest.h>

#include <cstdlib>

namespace wandb {
namespace {

// Test fixture for online integration tests.
// Requires a valid WANDB_API_KEY in .env or the environment.
class RunOnlineTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    PyRuntime::instance().initialize();

    // Login loads .env via python-dotenv, then authenticates with wandb.
    Run::login();

    // Verify the API key is available — fail immediately if not.
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

// Helper: creates a RunConfig for online integration testing.
RunConfig make_online_config() {
  RunConfig cfg;
  cfg.project = "wandb_client_test";
  cfg.mode = "online";
  return cfg;
}

// Verifies that a run can be initialized online with valid credentials.
TEST_F(RunOnlineTest, OnlineRunInit) {
  auto run = Run::init(make_online_config());
  EXPECT_FALSE(run.id().empty());
  EXPECT_FALSE(run.name().empty());
  EXPECT_FALSE(run.url().empty());
  run.finish();
}

// Verifies that logging metrics works against the live service.
TEST_F(RunOnlineTest, OnlineRunLogMetrics) {
  auto run = Run::init(make_online_config());
  EXPECT_NO_THROW(run.log({{"test_loss", 0.42}, {"test_acc", 0.88}}));
  EXPECT_NO_THROW(run.log({{"test_loss", 0.35}, {"test_acc", 0.91}}));
  run.finish();
}

// Verifies that setting summary metrics works against the live service.
TEST_F(RunOnlineTest, OnlineRunSummary) {
  auto run = Run::init(make_online_config());
  EXPECT_NO_THROW(run.set_summary("best_test_loss", 0.35));
  EXPECT_NO_THROW(run.set_summary("best_test_acc", 0.91));
  run.finish();
}

} // namespace
} // namespace wandb
