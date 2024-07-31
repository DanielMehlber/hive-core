#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>

namespace services {

/**
 * Possible success and error response states
 */
enum ServiceResponseStatus {
  /** Successful service execution */
  OK = 0,
  /** Parameters were missing or of wrong type */
  PARAMETER_ERROR = 10,
  /** Service failed due to internal errors */
  INTERNAL_ERROR = 20,
  /** Service implementation is not longer available */
  GONE = 30
};

/**
 * Data object containing the response from a service
 */
class ServiceResponse {
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
  template <typename T> void SetResult(const std::string &name, T value);

  std::optional<std::string> GetResult(const std::string &name);

  std::set<std::string> GetResultNames() const;

  ServiceResponseStatus GetStatus() const;
  void SetStatus(ServiceResponseStatus mStatus);

  std::string GetStatusMessage() const;
  void SetStatusMessage(const std::string &mStatusMessage);

  std::string GetTransactionId() const;
};

template <typename T>
inline void ServiceResponse::SetResult(const std::string &name, T value) {
  m_result_values[name] = std::to_string(value);
}

template <>
inline void ServiceResponse::SetResult<std::string>(const std::string &name,
                                                    std::string value) {
  m_result_values[name] = std::move(value);
}

inline std::string ServiceResponse::GetTransactionId() const {
  return m_transaction_id;
}

typedef std::shared_ptr<ServiceResponse> SharedServiceResponse;

} // namespace services
