#include "wandb_client/config.h"

#include <gtest/gtest.h>

namespace wandb {
namespace {

// Test fixture for RunConfig struct tests.
// No Python runtime needed — RunConfig is a pure C++ type.
class ConfigTest : public ::testing::Test {};

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

// Verifies that ConfigValue variant can hold all supported types.
TEST_F(ConfigTest, ConfigValueVariantTypes) {
  ConfigValue v_int = 42;
  EXPECT_EQ(std::get<int>(v_int), 42);

  ConfigValue v_double = 3.14;
  EXPECT_DOUBLE_EQ(std::get<double>(v_double), 3.14);

  ConfigValue v_bool = true;
  EXPECT_TRUE(std::get<bool>(v_bool));

  ConfigValue v_str = std::string("hello");
  EXPECT_EQ(std::get<std::string>(v_str), "hello");
}

// Verifies that stats_sampling_interval defaults to 0.0.
TEST_F(ConfigTest, StatsIntervalDefault) {
  RunConfig cfg;
  EXPECT_DOUBLE_EQ(cfg.stats_sampling_interval, 0.0);
}

// Verifies that stats_sampling_interval can be set.
TEST_F(ConfigTest, StatsIntervalSet) {
  RunConfig cfg;
  cfg.stats_sampling_interval = 5.0;
  EXPECT_DOUBLE_EQ(cfg.stats_sampling_interval, 5.0);
}

} // namespace
} // namespace wandb
