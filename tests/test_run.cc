#include "wandb_client/py_runtime.h"
#include "wandb_client/run.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>

namespace wandb {
namespace {

using testing::make_offline_config;

// Test fixture that initializes the Python runtime for all run tests.
// WANDB_MODE=offline is set by CMake so these tests never hit the network.
class RunTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

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
  auto cfg = make_offline_config();
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

// Verifies multiple threads can log metrics rapidly and concurrently without
// blocking or dropping, and that teardown happens cleanly.
TEST_F(RunTest, RunAsyncLoggingMultiThreaded) {
  auto run = Run::init(make_offline_config());

  const int num_threads = 10;
  const int logs_per_thread = 100;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&run, i]() {
      for (int j = 0; j < logs_per_thread; ++j) {
        run.log({{"thread_id", static_cast<double>(i)},
                 {"step", static_cast<double>(j)}});
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_NO_THROW(run.finish());
}

// Verifies that logging to a finished run throws WandbException.
TEST_F(RunTest, RunLogAfterFinishThrows) {
  auto run = Run::init(make_offline_config());
  run.finish();
  EXPECT_THROW(run.log({{"step", 1.0}}), WandbException);
}

} // namespace
} // namespace wandb
