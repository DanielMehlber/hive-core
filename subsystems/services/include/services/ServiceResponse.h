#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace hive::services {

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
  GONE = 30,
  /** The limit of concurrent calls for this service is reached. Try again
     later or call another exector. */
  BUSY = 40,
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

  /**
   * Captures the amount of attempts and retries that were necessary to resolve
   * the request.
   * @note Sometimes a service execution fails or the executor/node is busy.
   * Depending on the call's retry policy, the call is retried, increasing the
   * amount of attempts.
   */
  size_t m_resolution_attempts;

public:
  /**
   * Creates a new service response object.
   * @param transaction_id a request and its response have the same transaction
   * id which correlates them.
   * @param status status of the response.
   * @param status_message additional information about the status.
   * @param resolution_attempts amount of attempts and retries of service
   * executions that were necessary to resolve the request.
   */
  explicit ServiceResponse(std::string transaction_id,
                           ServiceResponseStatus status = OK,
                           std::string status_message = "",
                           size_t resolution_attempts = 1);

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

  /**
   * Get the amount of attempts and retries of service executions that were
   * necessary to resolve the request. Sometimes a service execution fails or
   * the executor/node is busy. Depending on the call's retry policy, the call
   * is retried, increasing the amount of attempts.
   * @return The amount of attempts and retries.
   */
  size_t GetResolutionAttempts() const;

  /**
   * Sets amount of attempts and retries of service executions that were
   * necessary to resolve the request. Sometimes a service execution fails or
   * the executor/node is busy. Depending on the call's retry policy, the call
   * is retried, increasing the amount of attempts.
   * @param attempts The amount of attempts and retries.
   */
  void SetResolutionAttempts(size_t attempts);
};

template <typename T>
void ServiceResponse::SetResult(const std::string &name, T value) {
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

inline size_t ServiceResponse::GetResolutionAttempts() const {
  return m_resolution_attempts;
}

inline void ServiceResponse::SetResolutionAttempts(size_t attempts) {
  m_resolution_attempts = attempts;
}

typedef std::shared_ptr<ServiceResponse> SharedServiceResponse;

} // namespace hive::services
