#ifndef SERVICERESPONSE_H
#define SERVICERESPONSE_H

#include "Services.h"
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace services {

/**
 * Possible success and error response states
 */
enum ServiceResponseStatus {
  /** Successful service execution */
  OK,
  /** Parameters were missing or of wrong type */
  PARAMETER_ERROR,
  /** Service failed due to internal errors */
  INTERNAL_ERROR,
  /** Service implementation is not longer available */
  GONE
};

/**
 * Data object containing the response from a service
 */
class SERVICES_API ServiceResponse {
protected:
  /**
   * Transaction ID that binds request and response together
   */
  const std::string m_transaction_id;

  /**
   * Response status code
   */
  ServiceResponseStatus m_status;

  /**
   * Message explaining the response status
   * @note may be empty in case of success
   */
  std::string m_status_message;

  /**
   * Result values mapped to their names
   */
  std::map<std::string, std::string> m_result_values;

public:
  explicit ServiceResponse(std::string transaction_id,
                           ServiceResponseStatus status = OK,
                           std::string status_message = "");

  /**
   * Sets a named result value
   * @tparam T type of the result value
   * @param name of the result value
   * @param value of the result
   */
  template <typename T> void SetResult(const std::string &name, const T &value);

  std::optional<std::string> GetResult(const std::string &name);

  ServiceResponseStatus GetStatus() const;
  void SetStatus(ServiceResponseStatus mStatus);
  std::string GetStatusMessage() const;
  void SetStatusMessage(const std::string &mStatusMessage);
};

template <typename T>
void ServiceResponse::SetResult(const std::string &name, const T &value) {
  std::stringstream ss;
  ss << value;
  m_result_values[name] = ss.str();
}

typedef std::shared_ptr<ServiceResponse> SharedServiceResponse;

} // namespace services

#endif // SERVICERESPONSE_H
