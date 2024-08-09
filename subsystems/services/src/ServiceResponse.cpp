#include "services/ServiceResponse.h"

using namespace hive::services;

ServiceResponse::ServiceResponse(std::string transaction_id,
                                 ServiceResponseStatus status,
                                 std::string status_message, size_t attempts)
    : m_transaction_id(std::move(transaction_id)), m_status(status),
      m_status_message(std::move(status_message)),
      m_resolution_attempts(attempts) {}

std::optional<std::string> ServiceResponse::GetResult(const std::string &name) {
  if (m_result_values.contains(name)) {
    return m_result_values.at(name);
  } else {
    return {};
  }
}

ServiceResponseStatus ServiceResponse::GetStatus() const { return m_status; }

void ServiceResponse::SetStatus(ServiceResponseStatus mStatus) {
  m_status = mStatus;
}

std::string ServiceResponse::GetStatusMessage() const {
  return m_status_message;
}

void ServiceResponse::SetStatusMessage(const std::string &mStatusMessage) {
  m_status_message = mStatusMessage;
}

std::set<std::string> ServiceResponse::GetResultNames() const {
  std::set<std::string> set;
  for (const auto &items : m_result_values) {
    set.insert(items.first);
  }

  return std::move(set);
}