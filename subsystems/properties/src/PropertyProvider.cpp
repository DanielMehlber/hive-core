#include "properties/PropertyProvider.h"
#include "messaging/MessagingFactory.h"
#include <sstream>
#include <vector>

using namespace props;

PropertyProvider::PropertyProvider(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {}

PropertyProvider::~PropertyProvider() = default;

/*
 * if the path is 'this.is.an.example', sub-paths are
 * - 'this'
 * - 'this.is'
 * - 'this.is.an'
 * - 'this.is.an.eample'
 */
std::vector<std::string> getSubpaths(const std::string &path, char delimeter) {
  std::vector<std::string> subpaths;

  if (!path.empty()) {
    size_t index = 0;
    while (true) {
      index = path.find_first_of(delimeter, index + 1);
      if (index == std::string::npos) {
        break;
      }
      std::string subpath = path.substr(0, index);
      subpaths.push_back(subpath);
    }

    subpaths.push_back(path);
  }

  return subpaths;
}

std::string buildTopicName(const std::string &path) {
  std::stringstream stream;
  stream << "prop-change(" << path << ")";
  return stream.str();
}

void PropertyProvider::RegisterListener(
    const std::string &path, const SharedPropertyListener &listener) {
  std::shared_ptr<messaging::IMessageSubscriber> subscriber =
      std::static_pointer_cast<messaging::IMessageSubscriber>(listener);

  auto message_broker =
      m_subsystems.lock()->RequireSubsystem<messaging::IMessageBroker>();
  message_broker->AddSubscriber(subscriber, buildTopicName(path));
}

void PropertyProvider::UnregisterListener(
    const SharedPropertyListener &listener) {
  std::shared_ptr<messaging::IMessageSubscriber> subscriber =
      std::static_pointer_cast<messaging::IMessageSubscriber>(listener);

  auto message_broker =
      m_subsystems.lock()->RequireSubsystem<messaging::IMessageBroker>();
  message_broker->RemoveSubscriber(subscriber);
}

void PropertyProvider::NotifyListenersAboutChange(
    const std::string &path) const {
  std::vector<std::string> paths = getSubpaths(path, '.');

  for (const auto &subpath : paths) {
    messaging::SharedMessage message =
        messaging::MessagingFactory::CreateMessage(buildTopicName(subpath));

    message->SetPayload("property-key", path);

    auto message_broker =
        m_subsystems.lock()->RequireSubsystem<messaging::IMessageBroker>();
    message_broker->PublishMessage(message);
  }
}
