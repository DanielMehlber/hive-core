#include <gtest/gtest.h>
#include <logging/Logging.h>

int main(int argc, char **argv) {
  auto log = logging::Logging::GetLogger();
  log->Info("Was soll das denn?");

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}