#ifndef PROPERTYCHANGELISTENERTEST_H
#define PROPERTYCHANGELISTENERTEST_H

#include "logging/Logging.h"
#include "properties/PropertyChangeListener.h"
#include "properties/PropertyProvider.h"
#include <gtest/gtest.h>

using namespace props;

class VerificationListener : public PropertyChangeListener {
private:
  unsigned int m_change_notification_received{0};

public:
  void ProcessPropertyChange(const std::string &key) noexcept override {
    m_change_notification_received++;
    LOG_DEBUG("received property change notification for "
              << key << " for a total of " << m_change_notification_received
              << " times");
  }

  unsigned int GetChangeNotificationsReceived() const {
    return m_change_notification_received;
  }
};

TEST(PropertyTest, simple_prop_listener) {
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();
  messaging::SharedBroker broker =
      messaging::MessagingFactory::CreateBroker(job_manager);
  PropertyProvider provider(broker);

  auto listener = std::make_shared<VerificationListener>();
  provider.RegisterListener("example.prop.value", listener);

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);
  provider.Set("example.prop.value", 1);
  job_manager->InvokeCycleAndWait();

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 1);
}

TEST(PropertyTest, sub_prop_listener) {
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();
  messaging::SharedBroker broker =
      messaging::MessagingFactory::CreateBroker(job_manager);
  PropertyProvider provider(broker);

  auto listener = std::make_shared<VerificationListener>();
  provider.RegisterListener("example.prop", listener);

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);
  provider.Set("example.prop.value", 1);
  provider.Set("example.prop.other", 1.0f);
  job_manager->InvokeCycleAndWait();

  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 2);
}

TEST(PropertyTest, listener_destroyed) {
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();
  messaging::SharedBroker broker =
      messaging::MessagingFactory::CreateBroker(job_manager);
  PropertyProvider provider(broker);

  // this just assures that there is no segfault happening when the listener is
  // deleted because it went out of scope.
  {
    auto listener = std::make_shared<VerificationListener>();
    provider.RegisterListener("example.prop", listener);
  }

  provider.Set("example.prop.value", 1);
  job_manager->InvokeCycleAndWait();
}

TEST(PropertyTest, listener_unregistered) {
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();
  messaging::SharedBroker broker =
      messaging::MessagingFactory::CreateBroker(job_manager);
  PropertyProvider provider(broker);

  // this just assures that there is no segfault happening when the listener is
  // deleted because it went out of scope.

  auto listener = std::make_shared<VerificationListener>();
  provider.RegisterListener("example.prop1", listener);
  provider.RegisterListener("exmaple.prop2", listener);
  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);

  provider.UnregisterListener(listener);
  provider.Set("example.prop1.value", 1);
  provider.Set("example.prop2.value", 1);
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(listener->GetChangeNotificationsReceived(), 0);
}

#endif /* PROPERTYCHANGELISTENERTEST_H */
