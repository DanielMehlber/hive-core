#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "logging/LogManager.h"
#include <memory>
#include <typeinfo>

namespace resourcemgmt {

DECLARE_EXCEPTION(WrongResourceTypeRequestedException);

class Resource {
private:
  const std::shared_ptr<void> m_value;
  const std::string m_type_name;
  const size_t m_type_id;

public:
  Resource() = delete;

  template <typename ResourceType>
  explicit Resource(std::shared_ptr<ResourceType> content_ptr);

  template <typename Type> std::shared_ptr<Type> ExtractAsType();
};

template <typename Type> std::shared_ptr<Type> Resource::ExtractAsType() {
  if (typeid(Type).hash_code() != m_type_id) {
    LOG_WARN("resource stores type " << m_type_name << ", but "
                                     << typeid(Type).name() << " was requested")
    THROW_EXCEPTION(WrongResourceTypeRequestedException,
                    "wrong resource type was requested")
  }

  return std::static_pointer_cast<Type>(m_value);
}

template <typename ResourceType>
inline Resource::Resource(std::shared_ptr<ResourceType> content_ptr)
    : m_value{std::static_pointer_cast<void>(content_ptr)},
      m_type_id{typeid(ResourceType).hash_code()},
      m_type_name(typeid(ResourceType).name()) {}

typedef std::shared_ptr<Resource> SharedResource;

} // namespace resourcemgmt
