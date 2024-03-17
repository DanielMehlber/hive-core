#include "properties/PropertyProvider.h"
#include <sstream>
#include <vector>

using namespace props;

PropertyProvider::PropertyProvider(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems)
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
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    std::shared_ptr<events::IEventListener> subscriber =
        std::static_pointer_cast<events::IEventListener>(listener);

    auto message_broker = subsystems->RequireSubsystem<events::IEventBroker>();
    message_broker->RegisterListener(subscriber, buildTopicName(path));
  } else /* if subsystems are not available */ {
    LOG_ERR("cannot register property change listener for '"
            << path << "' because required subsystems are not available")
  }
}

void PropertyProvider::UnregisterListener(
    const SharedPropertyListener &listener) {
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    std::shared_ptr<events::IEventListener> subscriber =
        std::static_pointer_cast<events::IEventListener>(listener);

    auto message_broker = subsystems->RequireSubsystem<events::IEventBroker>();
    message_broker->UnregisterListener(subscriber);
  } else /* if subsystems are not available */ {
    LOG_ERR("cannot unregister property change listener because required "
            "subsystems are not available or have been shut down already.")
  }
}

void PropertyProvider::NotifyListenersAboutChange(const std::string &path) {
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    std::vector<std::string> paths = getSubpaths(path, '.');

    for (const auto &subpath : paths) {
      auto event = std::make_shared<events::Event>(buildTopicName(subpath));
      event->SetPayload("property-key", path);

      auto message_broker =
          subsystems->RequireSubsystem<events::IEventBroker>();
      message_broker->FireEvent(event);
    }
  } else /* if subsystems are not available */ {
    LOG_ERR("cannot notify property change listeners of '"
            << path
            << "' because required subsystems are not available or have been "
               "shut down")
  }
}