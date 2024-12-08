#pragma once

#include "jobsystem/execution/JobExecution.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace hive::services {

/**
 * Request for a specific service containing parameters that the service
 * requires to do its work.
 */
class ServiceRequest {
protected:
  /**
   * unique name of the requested service
   */
  const std::string m_service_name;

  /**
   * Transaction ID that binds request and response together
   */
  const std::string m_transaction_id;

  /**
   * key-value parameters that are sent to the service implementation
   */
  std::map<std::string, std::string> m_parameters;

  /**
   * Each request instance can only be processed once at a time due to its
   * unique transactions id. Double processing the same request while it is
   * already pending can lead to unwanted behavior because the executors cannot
   * distinguish between two requests with the same transaction id.
   * @note This is a safety mechanism to prevent unwanted behavior. If the
   * request parameters should be transmitted again, this request must be
   * duplicatd.
   */
  bool m_is_currently_processed{false};
  mutable jobsystem::mutex m_currently_processed_mutex;

public:
  explicit ServiceRequest(std::string service_name);
  ServiceRequest(std::string service_name, std::string transaction_id);
  ServiceRequest(const ServiceRequest &other);

  /**
   * Duplicates this request and makes it usable again by assigning it a new
   * transaction id.
   * @return a copy of this request with another transaction id.
   */
  std::shared_ptr<ServiceRequest> Duplicate() const;

  /**
   * Set parameter value of request
   * @tparam T type of parameter (will be converted to string)
   * @param name name of parameter
   * @param t value of parameter
   */
  template <typename T> void SetParameter(const std::string &name, const T &t);

  /**
   * GetAsInt parameter value (if parameter has been set)
   * @param name identifier of parameter in request
   * @return parameter value as string (if it exists)
   */
  std::optional<std::string> GetParameter(const std::string &name) const;

  std::string GetServiceName() const;
  std::string GetTransactionId() const;

  std::set<std::string> GetParameterNames() const;

  bool IsCurrentlyProcessed() const;
  void MarkAsCurrentlyProcessed(bool currently_processed);
};

template <typename T>
inline void ServiceRequest::SetParameter(const std::string &name, const T &t) {
  m_parameters[name] = std::to_string(t);
}

template <>
inline void ServiceRequest::SetParameter<std::string>(const std::string &name,
                                                      const std::string &t) {
  m_parameters[name] = t;
}

inline bool ServiceRequest::IsCurrentlyProcessed() const {
  std::unique_lock lock(m_currently_processed_mutex);
  return m_is_currently_processed;
}

typedef std::shared_ptr<ServiceRequest> SharedServiceRequest;

} // namespace hive::services
