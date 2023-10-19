#include "services/ServiceRequest.h"
#include "common/uuid/UuidGenerator.h"

using namespace services;

ServiceRequest::ServiceRequest(std::string service_name)
    : m_service_name(std::move(service_name)),
      m_transaction_id(common::uuid::UuidGenerator::Random()) {}

std::optional<std::string>
ServiceRequest::GetParameter(const std::string &name) {
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
