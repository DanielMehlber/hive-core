#include "resourcemgmt/manager/impl/ThreadPoolResourceManager.h"
#include "logging/LogManager.h"

using namespace resourcemgmt;

void ThreadPoolResourceManager::RegisterLoader(
    std::shared_ptr<IResourceLoader> loader) {
  std::unique_lock registered_loaders_lock(m_registered_loaders_mutex);
  if (m_registered_loaders.contains(loader->GetId())) {
    THROW_EXCEPTION(DuplicateLoaderIdException,
                    "loader id '" << loader->GetId()
                                  << "' is already taken and must stay unique")
  }
  m_registered_loaders[loader->GetId()] = loader;
  LOG_DEBUG("new resource loader '" << loader->GetId() << "' registered")
}

void ThreadPoolResourceManager::UnregisterLoader(const std::string &id) {
  std::unique_lock registered_loaders_lock(m_registered_loaders_mutex);
  m_registered_loaders.erase(id);
  LOG_DEBUG("resource loader '" << id << "' unregistered")
}

void ThreadPoolResourceManager::LoadResourcesWorker() {
  while (true) {
    std::unique_lock queue_lock(m_loading_queue_mutex);
    m_loading_queue_condition.wait(queue_lock, [this]() {
      return this->m_terminate || !this->m_loading_queue.empty();
    });

    if (m_terminate) {
      return;
    }

    auto loading_function = m_loading_queue.front();
    m_loading_queue.pop();
    queue_lock.unlock();

    loading_function();
  }
}

ThreadPoolResourceManager::ThreadPoolResourceManager(
    size_t loading_thread_count)
    : m_terminate{false} {
  for (int i = 0; i < loading_thread_count; i++) {
    auto th =
        std::thread(&ThreadPoolResourceManager::LoadResourcesWorker, this);
    m_loading_threads.push_back(std::move(th));
  }
}

ThreadPoolResourceManager::~ThreadPoolResourceManager() {
  m_terminate = true;
  m_loading_queue_condition.notify_all();
  for (auto &th : m_loading_threads) {
    th.join();
  }
}

std::future<SharedResource>
ThreadPoolResourceManager::LoadResource(const std::string &uri) {
  /*
   * Example for 'file://myfile.txt'
   * - resource_loader_id = 'file'
   * - url = 'myfile.txt'
   * tells the resource manager to use the file loader to load 'myfile.txt'
   *
   * Other examples could be:
   * - 'ntfs://localhost:8080/file.txt'
   * - 'http://localhost:8080/file.txt'
   * - 'plugin://settings.xml'
   */
  const auto protocol_serperator_index = uri.find_first_of("://");
  std::string resource_loader_id;
  std::string resource_url;

  bool is_resource_loader_specified =
      protocol_serperator_index != std::string::npos;
  if (is_resource_loader_specified) {
    resource_loader_id = uri.substr(0, protocol_serperator_index);
    resource_url = uri.substr(protocol_serperator_index + 3);
  } else {
    resource_loader_id = "file";
    resource_url = uri;
  }

  std::unique_lock registered_loaders_lock(m_registered_loaders_mutex);
  bool resource_loader_exists =
      m_registered_loaders.contains(resource_loader_id);

  if (!resource_loader_exists) {
    LOG_ERR("unknown resource loader '" << resource_loader_id << "' requested")
    THROW_EXCEPTION(ResourceLoaderNotFound, "resource loader '"
                                                << resource_loader_id
                                                << "' does not exist")
  }

  auto resource_loader = m_registered_loaders[resource_loader_id];
  registered_loaders_lock.unlock();

  auto loading_promise = std::make_shared<std::promise<SharedResource>>();
  std::function<void()> loading_function = [loading_promise, resource_loader,
                                            resource_url] {
    try {
      SharedResource result_resource = resource_loader->Load(resource_url);
      loading_promise->set_value(result_resource);
    } catch (...) {
      loading_promise->set_exception(std::current_exception());
    }
  };

  // place loading function into queue
  std::unique_lock queue_lock(m_loading_queue_mutex);
  m_loading_queue.push(loading_function);
  m_loading_queue_condition.notify_one();
  queue_lock.unlock();

  return loading_promise->get_future();
}