#pragma once

#include "data/DataLayer.h"
#include "data/listeners/IDataChangeListener.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include <gtest/gtest.h>

using namespace hive::data;
using namespace hive::jobsystem;
using namespace hive;

class VerificationListener : public IDataChangeListener {
  std::atomic_int m_change_notification_received{0};

public:
  void Notify(const std::string &key, const std::string &value) override {
    m_change_notification_received++;
    LOG_DEBUG("received property change notification for "
              << key << " for a total of " << m_change_notification_received
              << " times")
  }

  int GetChangeNotificationsReceived() const {
    return m_change_notification_received.load();
  }
};

common::memory::Owner<common::subsystems::SubsystemManager> SetupSubsystems() {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);

  subsystems->AddOrReplaceSubsystem(std::move(job_manager));

  return subsystems;
}

TEST(PropertyTest, simple_change_listener) {
  auto subsystems = SetupSubsystems();
  auto job_manager = subsystems->RequireSubsystem<JobManager>();
  job_manager->StartExecution();

  auto data_layer = common::memory::Owner<DataLayer>(subsystems.Borrow());

  auto listener = std::make_shared<VerificationListener>();
  data_layer->RegisterListener("example.prop.value", listener);

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);
  data_layer->Set("example.prop.value", "1");
  job_manager->InvokeCycleAndWait();

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 1);
}

TEST(PropertyTest, sub_path_change_listener) {
  auto subsystems = SetupSubsystems();
  auto job_manager = subsystems->RequireSubsystem<JobManager>();
  job_manager->StartExecution();

  auto data_layer = common::memory::Owner<DataLayer>(subsystems.Borrow());

  auto listener = std::make_shared<VerificationListener>();
  data_layer->RegisterListener("example.prop", listener);

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);
  data_layer->Set("example.prop.value", "1");
  data_layer->Set("example.prop.other", "1.0f");
  job_manager->InvokeCycleAndWait();

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 2);
}

TEST(PropertyTest, listener_destroyed) {
  auto subsystems = SetupSubsystems();
  auto job_manager = subsystems->RequireSubsystem<JobManager>();
  job_manager->StartExecution();

  auto data_layer = common::memory::Owner<DataLayer>(subsystems.Borrow());

  // this just assures that there is no segfault happening when the listener is
  // deleted because it went out of scope.
  {
    auto listener = std::make_shared<VerificationListener>();
    data_layer->RegisterListener("example.prop", listener);
  }

  data_layer->Set("example.prop.value", "1");
  job_manager->InvokeCycleAndWait();
}
