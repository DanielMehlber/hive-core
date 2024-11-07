#include "data/DataLayer.h"

#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include <regex>

using namespace hive::data;
using namespace std::chrono_literals;
using namespace hive::jobsystem;

typedef std::vector<std::string> data_path_t;

DataLayer::DataLayer(
    common::memory::Borrower<common::subsystems::SubsystemManager> subsystems,
    const std::shared_ptr<IDataProvider> &provider)
    : m_provider(provider), m_subsystems(subsystems.ToReference()) {

  DEBUG_ASSERT(provider != nullptr, "provider implementation must not be null")

  std::weak_ptr<bool> is_alive = m_alive; // to track liveliness of data layer
  auto clean_up_job = std::make_shared<jobsystem::TimerJob>(
      [this, is_alive](jobsystem::JobContext *ctx) mutable {
        if (!is_alive.expired()) {
          std::unique_lock lock(this->m_listeners_mutex);

          // remove expired listeners
          for (auto &[_, listeners] : this->m_listeners) {
            std::erase_if(
                listeners,
                [](const std::weak_ptr<IDataChangeListener> &listener) {
                  return listener.expired();
                });
          }

          // remove empty listener lists
          std::erase_if(this->m_listeners,
                        [](const auto &entry) { return entry.second.empty(); });

          return jobsystem::JobContinuation::REQUEUE;
        }

        return jobsystem::JobContinuation::DISPOSE;
      },
      "data-layer-change-listeners-cleanup", 5s, jobsystem::CLEAN_UP);

  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(clean_up_job);
}

DataLayer::~DataLayer() = default;

void DataLayer::RegisterListener(
    const std::string &path,
    const std::weak_ptr<IDataChangeListener> &listener) {
  DEBUG_ASSERT(!path.empty(), "data path must not be empty")
  DEBUG_ASSERT(listener.lock() != nullptr, "listener must not be null")
  std::unique_lock lock(m_listeners_mutex);
  if (m_listeners.contains(path)) {
    m_listeners[path].push_back(listener);
  } else {
    m_listeners[path] = {listener};
  }

  LOG_DEBUG("registered listener for data pattern '" << path
                                                     << "' in data layer")
}

void DataLayer::SetProvider(const std::shared_ptr<IDataProvider> &&provider) {
  DEBUG_ASSERT(provider, "cannot set a null provider for data layer");
  std::unique_lock lock(m_provider_mutex);
  m_provider = provider;
}

void DataLayer::Set(const std::string &path, const std::string &data) {
  DEBUG_ASSERT(!path.empty(), "data path must not be empty")
  std::unique_lock lock(m_provider_mutex);
  DEBUG_ASSERT(m_provider, "no data provider specified for data layer");
  m_provider->Set(path, data);

  NotifyChange(path, data);
}

std::future<std::optional<std::string>>
DataLayer::Get(const std::string &path) const {
  std::unique_lock lock(m_provider_mutex);
  DEBUG_ASSERT(m_provider, "no data provider specified for data layer");
  return m_provider->Get(path);
}

data_path_t splitPath(const std::string &path) {
  // split path at each dot
  std::vector<std::string> path_parts;
  std::istringstream path_stream(path);
  std::string part;
  while (std::getline(path_stream, part, '.')) {
    path_parts.push_back(part);
  }

  return path_parts;
}

bool checkMatch(const data_path_t &path, const data_path_t &pattern) {
  const size_t path_size = path.size();
  const size_t pattern_size = pattern.size();

  if (path_size < pattern_size) {
    return false;
  }

  for (int i = 0; i < pattern_size; i++) {
    // begin search at the end of path (more likely to find differences)
    if (pattern[pattern_size - i - 1] != path[pattern_size - i - 1]) {
      return false;
    }
  }

  return true;
}

std::shared_ptr<Job>
createNotificationJob(const std::shared_ptr<IDataChangeListener> &listener,
                      const std::string &path, const std::string &data) {
  return std::make_shared<Job>(
      [listener, path, data](JobContext *) {
        listener->Notify(path, data);
        return DISPOSE;
      },
      "data-change-notification-{" + path + "}", MAIN);
}

void DataLayer::NotifyChange(const std::string &path, const std::string &data) {
  std::unique_lock lock(m_listeners_mutex);

  auto job_manager = m_subsystems.Borrow()->RequireSubsystem<JobManager>();

  const data_path_t path_parts = splitPath(path);
  for (const auto &[pattern, listeners] : m_listeners) {

    // quick and dirty optimization
    if (pattern[0] != path[0]) {
      continue;
    }

    const data_path_t pattern_path_parts = splitPath(pattern);
    if (checkMatch(path_parts, pattern_path_parts)) {
      for (const auto &listener : listeners) {
        if (const auto shared_listener = listener.lock()) {
          job_manager->KickJob(
              createNotificationJob(shared_listener, path, data));
        }
      }
    }
  }
}
