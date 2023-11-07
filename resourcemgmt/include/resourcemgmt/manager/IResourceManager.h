#ifndef IRESOURCEMANAGER_H
#define IRESOURCEMANAGER_H

#include "common/exceptions/ExceptionsBase.h"
#include "resourcemgmt/Resource.h"
#include "resourcemgmt/loader/IResourceLoader.h"
#include <future>
#include <memory>
#include <string>

namespace resourcemgmt {

DECLARE_EXCEPTION(DuplicateLoaderIdException);

class IResourceManager {
public:
  /**
   * @brief Registers loader and adds it to the list of other supported loaders
   * @note The loaders id must be unique or else this operation fails
   * @exception DuplicateLoaderIdException a loader with this id does already
   * exist.
   * @param loader new loader to register
   */
  virtual void RegisterLoader(std::shared_ptr<IResourceLoader> loader) = 0;

  /**
   * @brief Unregisters loader from supported loaders.
   * @param id id of loader
   */
  virtual void UnregisterLoader(const std::string &id) = 0;

  /**
   * @brief Loads Resource asynchronously
   * @param uri unique resource identifier consisting of loader id and resource
   * url.
   * @return future resource that will resolve as soon as the resource has been
   * loaded.
   */
  virtual std::future<SharedResource> LoadResource(const std::string &uri) = 0;
};

typedef std::shared_ptr<IResourceManager> SharedResourceManager;

} // namespace resourcemgmt

#endif /* IRESOURCEMANAGER_H */
