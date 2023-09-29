#include "properties/PropertyProvider.h"
#include <sstream>
#include <vector>

using namespace props;

PropertyProvider::PropertyProvider(messaging::SharedBroker message_broker)
    : m_message_broker{message_broker} {}

PropertyProvider::~PropertyProvider() {}

/*
 * if the path is 'this.is.an.example', sub-paths are
 * - 'this'
 * - 'this.is'
 * - 'this.is.an'
 * - 'this.is.an.eample'
 */
std::vector<std::string> getSubpaths(std::string path, char delimeter) {
  std::vector<std::string> subpaths;

  if (path.size() > 0) {
    size_t index = 0;
    while (true) {
      index = path.find_first_of(delimeter, index + 1);
      if (index == std::string::npos) {
        break;
      }
      std::string subpath = path.substr(0, index);
      subpaths.push_back(subpath);
    };

    subpaths.push_back(path);
  }

  return subpaths;
}

std::string buildTopicName(const std::string &path) {
  std::stringstream stream;
  stream << "prop-change(" << path << ")";
  return stream.str();
}

void PropertyProvider::RegisterListener(const std::string &path,
                                        SharedPropertyListener listener) {
  std::shared_ptr<messaging::IMessageSubscriber> subscriber =
      std::static_pointer_cast<messaging::IMessageSubscriber>(listener);

  m_message_broker->AddSubscriber(subscriber, buildTopicName(path));
}

void PropertyProvider::UnregisterListener(SharedPropertyListener listener) {
  std::shared_ptr<messaging::IMessageSubscriber> subscriber =
      std::static_pointer_cast<messaging::IMessageSubscriber>(listener);

  m_message_broker->RemoveSubscriber(subscriber);
}

void PropertyProvider::NotifyListenersAboutChange(
    const std::string &path) const {
  std::vector<std::string> paths = getSubpaths(path, '.');

  for (const auto &subpath : paths) {
    messaging::SharedMessage message =
        messaging::MessagingFactory::CreateMessage(buildTopicName(subpath));

    message->SetPayload("property-key", path);
    m_message_broker->PublishMessage(message);
  }
}
