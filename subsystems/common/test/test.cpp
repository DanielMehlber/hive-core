#include "ExclusiveOwnershipTest.h"
#include "common/subsystems/SubsystemManager.h"
#include <gtest/gtest.h>

using namespace hive::common::subsystems;
using namespace hive::common::memory;

TEST(SubsystemTest, subsystems) {
  // I was too lazy to write a subsystem, take a string instead
  auto system = Owner<std::string>("Hallo");

  SubsystemManager subsystems;
  subsystems.AddOrReplaceSubsystem<std::string>(std::move(system));

  ASSERT_EQ("Hallo", *subsystems.GetSubsystem<std::string>().value());
  ASSERT_THROW(subsystems.RequireSubsystem<SubsystemManager>(),
               SubsystemNotFoundException);
  ASSERT_THROW(subsystems.RemoveSubsystem<SubsystemManager>(),
               SubsystemNotFoundException);

  subsystems.RemoveSubsystem<std::string>();
  ASSERT_FALSE(subsystems.ProvidesSubsystem<std::string>());
  ASSERT_THROW(subsystems.RequireSubsystem<std::string>(),
               SubsystemNotFoundException);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}