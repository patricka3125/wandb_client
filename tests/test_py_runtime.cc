#include "wandb_client/py_runtime.h"

#include <gtest/gtest.h>

namespace wandb {
namespace {

// Test fixture that ensures the Python runtime is initialized once for all
// tests in this file and finalized after all tests complete.
class PyRuntimeTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

// Verifies that the interpreter reports as initialized after initialize().
TEST_F(PyRuntimeTest, InterpreterInitializes) {
  EXPECT_TRUE(PyRuntime::instance().is_initialized());
}

// Verifies that calling initialize() a second time is a safe no-op.
TEST_F(PyRuntimeTest, DoubleInitializeIsNoop) {
  EXPECT_NO_THROW(PyRuntime::instance().initialize());
  EXPECT_TRUE(PyRuntime::instance().is_initialized());
}

// Verifies that the 'sys' module can be imported through the cached import.
TEST_F(PyRuntimeTest, ImportSysModule) {
  py::object sys = PyRuntime::instance().import("sys");
  EXPECT_FALSE(sys.is_none());
}

// Verifies that importing 'wandb' succeeds (the venv must have it installed).
TEST_F(PyRuntimeTest, ImportWandb) {
  py::object wandb_mod = PyRuntime::instance().import("wandb");
  EXPECT_FALSE(wandb_mod.is_none());

  // Verify we can read the wandb version attribute.
  std::string version = wandb_mod.attr("__version__").cast<std::string>();
  EXPECT_FALSE(version.empty());
}

// Verifies that importing a non-existent module throws WandbException.
TEST_F(PyRuntimeTest, ImportNonexistentModuleThrows) {
  EXPECT_THROW(PyRuntime::instance().import("__nonexistent_module_xyz__"),
               WandbException);
}

// Verifies that repeated imports of the same module return the cached object.
TEST_F(PyRuntimeTest, ImportCachesModules) {
  py::object first = PyRuntime::instance().import("sys");
  py::object second = PyRuntime::instance().import("sys");
  EXPECT_TRUE(first.is(second));
}

// Verifies that GILGuard can be constructed and destructed without errors.
TEST_F(PyRuntimeTest, GILGuardAcquireRelease) {
  EXPECT_NO_THROW({
    GILGuard gil;
    // GIL is held here; basic Python operation should work.
    py::object none = py::none();
    EXPECT_TRUE(none.is_none());
  });
}

} // namespace
} // namespace wandb
