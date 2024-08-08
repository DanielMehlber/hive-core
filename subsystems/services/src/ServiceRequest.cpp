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

ServiceRequest::ServiceRequest(const ServiceRequest &other)
    : m_service_name(other.m_service_name),
      m_transaction_id(common::uuid::UuidGenerator::Random()),
      m_parameters(other.m_parameters) {}

std::shared_ptr<ServiceRequest> ServiceRequest::Duplicate() const {
  ServiceRequest request(*this);
  return std::make_shared<ServiceRequest>(std::move(request));
}

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

void ServiceRequest::MarkAsCurrentlyProcessed(bool currently_processed) {
  std::unique_lock lock(m_currently_processed_mutex);
  DEBUG_ASSERT(currently_processed ? !m_is_currently_processed : true,
               "Simuntaneous processing of the same request is prohibited. "
               "Duplicate the request for that purpose or wait "
               "until it has been resolved.")
  m_is_currently_processed = currently_processed;
}
