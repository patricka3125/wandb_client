#include "wandb_client/config.h"
#include "wandb_client/py_runtime.h"

#include <gtest/gtest.h>

namespace wandb {
namespace {

// Test fixture that ensures the Python runtime is initialized.
class ConfigTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { PyRuntime::instance().initialize(); }

  static void TearDownTestSuite() { PyRuntime::instance().finalize(); }
};

// Verifies that a default-constructed RunConfig has empty optional fields.
TEST_F(ConfigTest, RunConfigDefaults) {
  RunConfig cfg;
  EXPECT_TRUE(cfg.project.empty());
  EXPECT_TRUE(cfg.entity.empty());
  EXPECT_TRUE(cfg.name.empty());
  EXPECT_TRUE(cfg.notes.empty());
  EXPECT_TRUE(cfg.tags.empty());
  EXPECT_TRUE(cfg.config.empty());
  EXPECT_TRUE(cfg.mode.empty());
  EXPECT_TRUE(cfg.dir.empty());
  EXPECT_DOUBLE_EQ(cfg.stats_sampling_interval, 0.0);
}

// Verifies that RunConfig fields can be set and read back correctly.
TEST_F(ConfigTest, RunConfigSetFields) {
  RunConfig cfg;
  cfg.project = "test-project";
  cfg.entity = "test-entity";
  cfg.name = "test-run";
  cfg.notes = "some notes";
  cfg.tags = {"tag1", "tag2"};
  cfg.config["lr"] = 0.001;
  cfg.config["epochs"] = 10;
  cfg.mode = "offline";
  cfg.dir = "/tmp/wandb";

  EXPECT_EQ(cfg.project, "test-project");
  EXPECT_EQ(cfg.entity, "test-entity");
  EXPECT_EQ(cfg.name, "test-run");
  EXPECT_EQ(cfg.notes, "some notes");
  EXPECT_EQ(cfg.tags.size(), 2u);
  EXPECT_EQ(cfg.config.size(), 2u);
  EXPECT_EQ(cfg.mode, "offline");
  EXPECT_EQ(cfg.dir, "/tmp/wandb");
}

// Verifies that each ConfigValue variant type converts to the correct Python
// type.
TEST_F(ConfigTest, ConfigValueVariant) {
  // int
  {
    ConfigValue v = 42;
    py::object obj = config_value_to_py(v);
    EXPECT_EQ(obj.cast<int>(), 42);
  }
  // double
  {
    ConfigValue v = 3.14;
    py::object obj = config_value_to_py(v);
    EXPECT_DOUBLE_EQ(obj.cast<double>(), 3.14);
  }
  // bool
  {
    ConfigValue v = true;
    py::object obj = config_value_to_py(v);
    EXPECT_TRUE(obj.cast<bool>());
  }
  // string
  {
    ConfigValue v = std::string("hello");
    py::object obj = config_value_to_py(v);
    EXPECT_EQ(obj.cast<std::string>(), "hello");
  }
}

// Verifies that config_to_py_dict includes all populated fields.
TEST_F(ConfigTest, ConfigToPyDict) {
  RunConfig cfg;
  cfg.project = "my-project";
  cfg.entity = "my-team";
  cfg.name = "run-1";
  cfg.notes = "testing conversion";
  cfg.tags = {"ml", "test"};
  cfg.config["lr"] = 0.01;
  cfg.config["batch_size"] = 32;
  cfg.mode = "offline";
  cfg.dir = "/tmp";

  py::dict d = config_to_py_dict(cfg);

  EXPECT_EQ(d["project"].cast<std::string>(), "my-project");
  EXPECT_EQ(d["entity"].cast<std::string>(), "my-team");
  EXPECT_EQ(d["name"].cast<std::string>(), "run-1");
  EXPECT_EQ(d["notes"].cast<std::string>(), "testing conversion");
  EXPECT_EQ(d["mode"].cast<std::string>(), "offline");
  EXPECT_EQ(d["dir"].cast<std::string>(), "/tmp");

  // tags should be a list
  py::list tags = d["tags"].cast<py::list>();
  EXPECT_EQ(tags.size(), 2u);

  // config should be a nested dict
  py::dict py_config = d["config"].cast<py::dict>();
  EXPECT_DOUBLE_EQ(py_config["lr"].cast<double>(), 0.01);
  EXPECT_EQ(py_config["batch_size"].cast<int>(), 32);
}

// Verifies that config_to_py_dict omits empty optional fields.
TEST_F(ConfigTest, ConfigToPyDictOmitsEmpty) {
  RunConfig cfg;
  cfg.project = "minimal";

  py::dict d = config_to_py_dict(cfg);

  EXPECT_TRUE(d.contains("project"));
  EXPECT_FALSE(d.contains("entity"));
  EXPECT_FALSE(d.contains("name"));
  EXPECT_FALSE(d.contains("notes"));
  EXPECT_FALSE(d.contains("tags"));
  EXPECT_FALSE(d.contains("config"));
  EXPECT_FALSE(d.contains("mode"));
  EXPECT_FALSE(d.contains("dir"));
  EXPECT_FALSE(d.contains("settings"));
}

// Verifies that stats_sampling_interval produces a settings key in the dict.
TEST_F(ConfigTest, ConfigToPyDictWithSettings) {
  RunConfig cfg;
  cfg.project = "settings-test";
  cfg.stats_sampling_interval = 5.0;

  py::dict d = config_to_py_dict(cfg);

  EXPECT_TRUE(d.contains("settings"));
}

// Verifies that settings key is absent when stats_sampling_interval is 0.
TEST_F(ConfigTest, ConfigToPyDictSettingsOmittedWhenZero) {
  RunConfig cfg;
  cfg.project = "no-settings";
  cfg.stats_sampling_interval = 0.0;

  py::dict d = config_to_py_dict(cfg);

  EXPECT_FALSE(d.contains("settings"));
}

} // namespace
} // namespace wandb
