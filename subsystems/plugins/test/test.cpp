#include "TestPlugin.h"
#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "jobsystem/manager/JobManager.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resources/manager/IResourceManager.h"
#include "resources/manager/impl/ThreadPoolResourceManager.h"
#include <gtest/gtest.h>

using namespace hive;

TEST(PluginsTest, lifecycle_test) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  auto job_manager_ref = job_manager.CreateReference();
  job_manager->StartExecution();

  auto resource_manager =
      common::memory::Owner<resources::ThreadPoolResourceManager>();

  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(std::move(job_manager));
  subsystems->AddOrReplaceSubsystem<resources::IResourceManager>(
      std::move(resource_manager));

  auto plugin_context = std::make_shared<plugins::PluginContext>(
      subsystems.CreateReference(), config);
  common::memory::Owner<plugins::IPluginManager> plugin_manager =
      common::memory::Owner<plugins::BoostPluginManager>(
          plugin_context, subsystems.CreateReference());

  auto plugin = boost::shared_ptr<TestPlugin>(new TestPlugin());

  plugin_manager->InstallPluginAsJob(plugin);
  plugin_manager->UnloadPluginAsJob(plugin->GetName());

  job_manager_ref.Borrow()->InvokeCycleAndWait();

  ASSERT_EQ(plugin->GetInitCount(), 1);
  ASSERT_EQ(plugin->GetShutdownCount(), 1);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}