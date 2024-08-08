#include "services/registry/impl/remote/RemoteServiceRequestConsumer.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace hive::services::impl;

RemoteServiceRequestConsumer::RemoteServiceRequestConsumer(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    RemoteServiceRequestConsumer::query_func_t query_func,
    const common::memory::Reference<IMessageEndpoint> &endpoint)
    : m_subsystems(subsystems), m_service_query_func(std::move(query_func)),
      m_endpoint(endpoint) {}

void RemoteServiceRequestConsumer::ProcessReceivedMessage(
    SharedMessage received_message, ConnectionInfo connection_info) {

  DEBUG_ASSERT(received_message != nullptr,
               "received message should not be null")
  DEBUG_ASSERT(!received_message->GetId().empty(),
               "received message should have an id")

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();

    auto maybe_request =
        RemoteServiceMessagesConverter::ToServiceRequest(received_message);

    if (!maybe_request.has_value()) {
      LOG_WARN("received invalid service request. Dropped.")
      return;
    }

    SharedServiceRequest request = maybe_request.value();
    LOG_DEBUG("received remote service request for service '"
              << request->GetServiceName() << "'")

    SharedJob job = jobsystem::JobSystemFactory::CreateJob(
        [_this = std::static_pointer_cast<RemoteServiceRequestConsumer>(
             shared_from_this()),
         request, connection_info](jobsystem::JobContext *context) {
          LOG_DEBUG("processing remote service request for service '"
                    << request->GetServiceName() << "'")

          auto job_manager = _this->m_subsystems.TryBorrow()
                                 .value()
                                 ->RequireSubsystem<jobsystem::JobManager>();

          auto future_service_caller =
              _this->m_service_query_func(request->GetServiceName(), true);

          context->GetJobManager()->WaitForCompletion(future_service_caller);

          SharedServiceResponse response;

          try {
            auto maybe_service_caller = future_service_caller.get();
            if (maybe_service_caller.has_value()) {
              auto service_caller = maybe_service_caller.value();

              // call service locally
              std::future<SharedServiceResponse> future_response =
                  service_caller->IssueCallAsJob(request, job_manager, true,
                                                 true);

              context->GetJobManager()->WaitForCompletion(future_response);
              response = future_response.get();

            } else {
              LOG_WARN("received remote service request for service '"
                       << request->GetServiceName()
                       << "' that does not exist locally")
              response = std::make_shared<ServiceResponse>(
                  request->GetTransactionId(), ServiceResponseStatus::GONE,
                  "service does not exist (anymore)");
            }
          } catch (std::exception &exception) {
            LOG_ERR("cannot execute service "
                    << request->GetServiceName()
                    << " called from remote endpoint: internal "
                       "error occurred "
                       "during local service execution: "
                    << exception.what())

            response = std::make_shared<ServiceResponse>(
                request->GetTransactionId(),
                ServiceResponseStatus::INTERNAL_ERROR, exception.what());
          }

          SharedMessage response_message =
              RemoteServiceMessagesConverter::FromServiceResponse(
                  std::move(*response));

          if (auto maybe_endpoint = _this->m_endpoint.TryBorrow()) {
            auto endpoint = maybe_endpoint.value();
            auto sending_progress =
                endpoint->Send(connection_info.endpoint_id, response_message);
            context->GetJobManager()->WaitForCompletion(sending_progress);

            try {
              sending_progress.get();
              LOG_DEBUG("sent remote service response for request "
                        << request->GetTransactionId() << " to node "
                        << connection_info.endpoint_id)
            } catch (std::exception &exception) {
              LOG_ERR("error while sending response for remote service request "
                      << request->GetTransactionId() << " for service "
                      << request->GetServiceName()
                      << " due to sending error: " << exception.what())
            }
          } else {
            LOG_ERR("cannot send service response for remote service request "
                    << request->GetTransactionId()
                    << " because web-socket peer has shut down")
          }

          return JobContinuation::DISPOSE;
        },
        "process-service-request-" + request->GetTransactionId(), MAIN, true);

    job_manager->KickJob(job);
  } else /* if subsystems are not available */ {
    if (!m_subsystems.CanBorrow()) {
      LOG_ERR("cannot process received message because required subsystems are "
              "not available or have been shut down")
    }
  }
}