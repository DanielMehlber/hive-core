#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "common/exceptions/ExceptionsBase.h"
#include "resourcemgmt/Resource.h"
#include "resourcemgmt/loader/IResourceLoader.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include <condition_variable>
#include <map>
#include <queue>

namespace resourcemgmt {

DECLARE_EXCEPTION(ResourceLoaderNotFound);

class ThreadPoolResourceManager : public IResourceManager {
private:
  std::map<std::string, std::shared_ptr<IResourceLoader>> m_registered_loaders;

  std::queue<std::function<void()>> m_loading_queue;
  std::condition_variable m_loading_queue_condition;
  std::mutex m_loading_queue_mutex;

  std::vector<std::shared_ptr<std::thread>> m_loading_threads;
  bool m_terminate{false};

  void LoadResourcesWorker();

public:
  ThreadPoolResourceManager(size_t loading_thread_count = 1);
  ~ThreadPoolResourceManager();

  virtual void RegisterLoader(std::shared_ptr<IResourceLoader> loader) override;
  virtual void UnregisterLoader(const std::string &id) override;

  virtual std::future<SharedResource>
  LoadResource(const std::string &uri) override;
};

} // namespace resourcemgmt

#endif /* RESOURCEMANAGER_H */
