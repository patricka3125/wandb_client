#include "wandb_client/metrics.h"
#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

#include <gtest/gtest.h>

#include <thread>

namespace wandb {
namespace {

// Test fixture that initializes the Python runtime.
// WANDB_MODE=offline is set by CMake.
class MetricsTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

// Helper: creates a minimal offline RunConfig.
RunConfig make_offline_config() {
  RunConfig cfg;
  cfg.project = "wandb_client_metrics_test";
  cfg.mode = "offline";
  return cfg;
}

// Verifies that elapsed_ms() returns a value close to the actual sleep time.
TEST_F(MetricsTest, TimerElapsedAccuracy) {
  auto run = Run::init(make_offline_config());
  TrainingTimer timer(run, "test_elapsed_ms");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  double ms = timer.elapsed_ms();
  EXPECT_GE(ms, 50.0);
  EXPECT_LE(ms, 200.0);
  timer.stop();
  run.finish();
}

// Verifies that the timer auto-logs on scope exit without throwing.
TEST_F(MetricsTest, TimerAutoLogs) {
  auto run = Run::init(make_offline_config());
  EXPECT_NO_THROW({
    TrainingTimer timer(run, "auto_log_ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  });
  run.finish();
}

// Verifies that manual stop() logs and destructor is a no-op.
TEST_F(MetricsTest, TimerManualStop) {
  auto run = Run::init(make_offline_config());
  TrainingTimer timer(run, "manual_stop_ms");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_NO_THROW(timer.stop());
  run.finish();
}

// Verifies that calling stop() twice is a safe no-op.
TEST_F(MetricsTest, TimerDoubleStopIsNoop) {
  auto run = Run::init(make_offline_config());
  TrainingTimer timer(run, "double_stop_ms");
  timer.stop();
  EXPECT_NO_THROW(timer.stop());
  run.finish();
}

} // namespace
} // namespace wandb
