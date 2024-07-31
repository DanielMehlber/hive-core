#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "jobsystem/synchronization/JobMutex.h"
#include "resourcemgmt/Resource.h"
#include "resourcemgmt/loader/IResourceLoader.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>

namespace resourcemgmt {

DECLARE_EXCEPTION(ResourceLoaderNotFound);

class ThreadPoolResourceManager : public IResourceManager {
private:
  std::map<std::string, std::shared_ptr<IResourceLoader>> m_registered_loaders;
  mutable jobsystem::mutex m_registered_loaders_mutex;

  std::queue<std::function<void()>> m_loading_queue;
  std::condition_variable m_loading_queue_condition;
  mutable std::mutex m_loading_queue_mutex;

  std::vector<std::thread> m_loading_threads;
  bool m_terminate{false};

  void LoadResourcesWorker();

public:
  explicit ThreadPoolResourceManager(size_t loading_thread_count = 1);
  ~ThreadPoolResourceManager();

  void RegisterLoader(std::shared_ptr<IResourceLoader> loader) override;
  void UnregisterLoader(const std::string &id) override;

  std::future<SharedResource> LoadResource(const std::string &uri) override;
};

} // namespace resourcemgmt
