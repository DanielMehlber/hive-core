#ifndef RESOURCE_H
#define RESOURCE_H

#include "common/exceptions/ExceptionsBase.h"
#include "logging/Logging.h"
#include <memory>
#include <typeinfo>

namespace resourcemgmt {

DECLARE_EXCEPTION(WrongResourceTypeRequestedException);

class Resource {
private:
  std::shared_ptr<void> m_value;
  size_t m_type_info;

public:
  Resource() = delete;

  template <typename ResourceType>
  Resource(std::shared_ptr<ResourceType> content_ptr);

  template <typename Type> std::shared_ptr<Type> ExtractAsType();
};

template <typename Type> std::shared_ptr<Type> Resource::ExtractAsType() {
  if (typeid(Type).hash_code() != m_type_info) {
    LOG_WARN("cannot access resource because wrong content type was requested");
    THROW_EXCEPTION(WrongResourceTypeRequestedException,
                    "wrong resource type was requested");
  }

  return std::static_pointer_cast<Type>(m_value);
}

template <typename ResourceType>
inline Resource::Resource(std::shared_ptr<ResourceType> content_ptr)
    : m_value{std::static_pointer_cast<void>(content_ptr)},
      m_type_info{typeid(ResourceType).hash_code()} {}

typedef std::shared_ptr<Resource> SharedResource;

} // namespace resourcemgmt

#endif /* RESOURCE_H */
