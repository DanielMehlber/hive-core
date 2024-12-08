#pragma once

#include "data/providers/IDataProvider.h"
#include <future>
#include <optional>

namespace hive::data {

class LocalDataProvider : public IDataProvider {
  /**
   * Pointer to implementation (Pimpl) in source file. This is necessary to
   * constrain Boost's implementation details in the source file and not expose
   * them to the rest of the application.
   * @note Indispensable for ABI stability and to use static-linked Boost.
   */
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  LocalDataProvider();
  ~LocalDataProvider() override;

  LocalDataProvider(const LocalDataProvider &other) = delete;
  LocalDataProvider &operator=(const LocalDataProvider &other) = delete;

  LocalDataProvider(LocalDataProvider &&other) noexcept;
  LocalDataProvider &operator=(LocalDataProvider &&other) noexcept;

  std::future<std::optional<std::string>> Get(const std::string &path) override;
  void Set(const std::string &path, const std::string &data) override;
};

} // namespace hive::data
