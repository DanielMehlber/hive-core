#include "data/DataLayer.h"

#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include <regex>

using namespace hive::data;
using namespace std::chrono_literals;

typedef std::vector<std::string> data_path_t;

DataLayer::DataLayer(
    common::memory::Borrower<common::subsystems::SubsystemManager> subsystems,
    const std::shared_ptr<IDataProvider> &provider)
    : m_provider(provider), m_subsystems(subsystems.ToReference()) {

  std::weak_ptr<bool> is_alive = m_alive;
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
  std::unique_lock lock(m_provider_mutex);
  DEBUG_ASSERT(provider, "cannot set a null provider for data layer");
  m_provider = provider;
}

void DataLayer::Set(const std::string &path, const std::string &data) const {
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

void DataLayer::NotifyChange(const std::string &path,
                             const std::string &data) const {
  std::unique_lock lock(m_listeners_mutex);
  const data_path_t path_parts = splitPath(path);
  for (const auto &[pattern, listeners] : m_listeners) {
    if (const data_path_t pattern_path_parts = splitPath(pattern);
        checkMatch(path_parts, pattern_path_parts)) {
      for (const auto &listener : listeners) {
        if (const auto shared_listener = listener.lock()) {
          shared_listener->Notify(path, data);
        }
      }
    }
  }
}
