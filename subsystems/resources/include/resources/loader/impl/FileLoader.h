#pragma once

#include "resources/loader/IResourceLoader.h"

namespace hive::resources::loaders {

/**
 * Loads files from the device file-system
 */
class FileLoader : public IResourceLoader {
public:
  /**
   * GetAsInt id of this resource loader. This id will be used to
   * determin the resource loader when the resource manager receives a resource
   * request.
   * @return id of this loader
   */
  const std::string &GetId() const  override;

  /**
   * Loads some resource of arbitrary type (e.g. from filesystem or
   * network) using the URI of the resource.
   * @note The implementation of this function may be blocking because it will
   * be wrapped in an asynchronous function.
   * @attention If the loading process fails, this function can throw arbitrary
   * exceptions depending on the implementation and concrete resource type.
   * @param uri resource locator
   * @return the loaded resource
   */
  SharedResource Load(const std::string &uri) override;
};

inline const std::string &FileLoader::GetId() const  {
  static std::string id = "file";
  return id;
}

} // namespace hive::resources::loaders
