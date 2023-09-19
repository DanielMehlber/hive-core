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

  virtual SharedResource Load(const std::string &uri) = 0;
};
} // namespace resourcemgmt

#endif /* IRESOURCELOADER_H */
