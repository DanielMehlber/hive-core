#ifndef SERVICEREQUEST_H
#define SERVICEREQUEST_H

#include "Services.h"
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace services {

/**
 * Request for a specific service containing parameters that the service
 * requires to do its work.
 */
class SERVICES_API ServiceRequest {
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

public:
  explicit ServiceRequest(std::string service_name);
  ServiceRequest(std::string service_name, std::string transaction_id);

  /**
   * Set parameter value of request
   * @tparam T type of parameter (will be converted to string)
   * @param name name of parameter
   * @param t value of parameter
   */
  template <typename T> void SetParameter(const std::string &name, const T &t);

  /**
   * Get parameter value (if parameter has been set)
   * @param name identifier of parameter in request
   * @return parameter value as string (if it exists)
   */
  std::optional<std::string> GetParameter(const std::string &name) const;

  std::string GetServiceName() const;
  std::string GetTransactionId() const;

  std::set<std::string> GetParameterNames() const;
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

typedef std::shared_ptr<ServiceRequest> SharedServiceRequest;

} // namespace services

#endif // SERVICEREQUEST_H
