#ifndef IRESOURCELOADER_H
#define IRESOURCELOADER_H

#include "resourcemgmt/Resource.h"
#include <any>
#include <future>
#include <memory>
#include <string>

namespace resourcemgmt {

/**
 * @brief Interface for a variety of resource loaders that will be used by the
 * resource manager.
 */
class IResourceLoader {
public:
  /**
   * @brief Get id of this resource loader. This id will be used to determin the
   * resource loader when the resource manager receives a resource request.
   * @return id of this loader
   */
  virtual const std::string &GetId() const noexcept = 0;

  /**
   * @brief Loads some resource of arbitrary type (e.g. from filesystem or
   * network) using the URI of the resource.
   * @note The implementation of this function may be blocking because it will
   * be wrapped in an asynchronous function.
   * @attention If the loading process fails, this function can throw arbitrary
   * exceptions depending on the implementation and concrete resource type.
   * @param uri resource locator
   * @return the loaded resource
   */
  virtual SharedResource Load(const std::string &uri) = 0;
};
} // namespace resourcemgmt

#endif /* IRESOURCELOADER_H */
