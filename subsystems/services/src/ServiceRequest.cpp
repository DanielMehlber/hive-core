#include "services/ServiceRequest.h"
#include "common/uuid/UuidGenerator.h"

using namespace hive::services;

ServiceRequest::ServiceRequest(std::string service_name)
    : m_service_name(std::move(service_name)),
      m_transaction_id(common::uuid::UuidGenerator::Random()) {}

ServiceRequest::ServiceRequest(std::string service_name,
                               std::string transaction_id)
    : m_service_name(std::move(service_name)),
      m_transaction_id(std::move(transaction_id)) {}

std::optional<std::string>
ServiceRequest::GetParameter(const std::string &name) const {
  if (m_parameters.contains(name)) {
    return m_parameters.at(name);
  } else {
    return {};
  }
}

std::string ServiceRequest::GetServiceName() const { return m_service_name; }

std::string ServiceRequest::GetTransactionId() const {
  return m_transaction_id;
}

std::set<std::string> ServiceRequest::GetParameterNames() const {
  std::set<std::string> set;
  for (const auto &elem : m_parameters) {
    set.insert(elem.first);
  }

  return std::move(set);
}
