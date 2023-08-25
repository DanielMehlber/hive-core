#include <gtest/gtest.h>
#include <jobsystem/JobApi.h>
#include <jobsystem/manager/IJobManager.h>
#include <logging/LoggingApi.h>

int main(int argc, char **argv) {
  auto log = logging::LoggingApi::GetLogger();
  log->Info("Was soll das denn?");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}