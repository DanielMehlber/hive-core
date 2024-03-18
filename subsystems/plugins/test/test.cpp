#include "TestPlugin.h"
#include "common/memory/ExclusiveOwnership.h"
#include "plugins/impl/BoostPluginManager.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "resourcemgmt/manager/impl/ThreadPoolResourceManager.h"
#include <gtest/gtest.h>

TEST(PluginsTest, lifecycle_test) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  auto job_manager_ref = job_manager.CreateReference();
  job_manager->StartExecution();

  auto resource_manager =
      common::memory::Owner<resourcemgmt::ThreadPoolResourceManager>();

  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(std::move(job_manager));
  subsystems->AddOrReplaceSubsystem<resourcemgmt::IResourceManager>(
      std::move(resource_manager));

  auto plugin_context =
      std::make_shared<plugins::PluginContext>(subsystems.CreateReference());
  common::memory::Owner<plugins::IPluginManager> plugin_manager =
      common::memory::Owner<plugins::BoostPluginManager>(
          plugin_context, subsystems.CreateReference());

  auto plugin = boost::shared_ptr<TestPlugin>(new TestPlugin());

  plugin_manager->InstallPlugin(plugin);
  plugin_manager->UninstallPlugin(plugin->GetName());

  job_manager_ref.Borrow()->InvokeCycleAndWait();

  ASSERT_EQ(plugin->GetInitCount(), 1);
  ASSERT_EQ(plugin->GetShutdownCount(), 1);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}