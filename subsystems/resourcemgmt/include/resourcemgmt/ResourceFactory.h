#ifndef RESOURCEFACTORY_H
#define RESOURCEFACTORY_H

#include "memory"
#include "resourcemgmt/Resource.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "resourcemgmt/manager/impl/ThreadPoolResourceManager.h"

#define DefaultResourceManagerImpl resourcemgmt::ThreadPoolResourceManager

namespace resourcemgmt {

class ResourceFactory {
public:
  template <typename ResourceType>
  static SharedResource
  CreateSharedResource(std::shared_ptr<ResourceType> content_ptr);

  template <typename ResourceType>
  static SharedResource CreateSharedResource(const ResourceType &content);

  template <typename ManagerType = DefaultResourceManagerImpl, typename... Args>
  static std::shared_ptr<IResourceManager> CreateResourceManager(Args... args);
};

template <typename ResourceType>
inline SharedResource resourcemgmt::ResourceFactory::CreateSharedResource(
    std::shared_ptr<ResourceType> content_ptr) {
  return std::make_shared<Resource>(content_ptr);
}

template <typename ResourceType>
inline SharedResource resourcemgmt::ResourceFactory::CreateSharedResource(
    const ResourceType &content) {
  auto ptr = std::make_shared<ResourceType>(content);
  return CreateSharedResource(ptr);
}

template <typename ManagerType, typename... Args>
inline std::shared_ptr<IResourceManager>
resourcemgmt::ResourceFactory::CreateResourceManager(Args... args) {
  return std::static_pointer_cast<IResourceManager>(
      std::make_shared<ManagerType>(args...));
}

} // namespace resourcemgmt

#endif /* RESOURCEFACTORY_H */
