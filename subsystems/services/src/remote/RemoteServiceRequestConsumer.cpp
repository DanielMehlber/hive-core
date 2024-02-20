#include "services/registry/impl/remote/RemoteServiceRequestConsumer.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace services::impl;

RemoteServiceRequestConsumer::RemoteServiceRequestConsumer(
    const common::subsystems::SharedSubsystemManager &subsystems,
    RemoteServiceRequestConsumer::query_func_type query_func,
    const SharedMessageEndpoint &web_socket_peer)
    : m_subsystems(subsystems), m_service_query_func(std::move(query_func)),
      m_web_socket_peer(web_socket_peer) {}

void RemoteServiceRequestConsumer::ProcessReceivedMessage(
    SharedMessage received_message, ConnectionInfo connection_info) noexcept {

  if (auto subsystems = m_subsystems.lock()) {
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();

    auto maybe_request =
        RemoteServiceMessagesConverter::ToServiceRequest(received_message);

    if (!maybe_request.has_value()) {
      LOG_WARN("received invalid service request. Dropped.")
      return;
    }

    SharedServiceRequest request = maybe_request.value();
    LOG_DEBUG("received service request for service "
              << request->GetServiceName())

    SharedJob job = jobsystem::JobSystemFactory::CreateJob(
        [_this = std::static_pointer_cast<RemoteServiceRequestConsumer>(
             shared_from_this()),
         request, connection_info](jobsystem::JobContext *context) {
          auto job_manager = _this->m_subsystems.lock()
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
                  service_caller->Call(request, job_manager, true);

              context->GetJobManager()->WaitForCompletion(future_response);
              response = future_response.get();

            } else {
              LOG_WARN("received service request for service '"
                       << request->GetServiceName()
                       << "' that does not exist locally")
              response = std::make_shared<ServiceResponse>(
                  request->GetTransactionId(), ServiceResponseStatus::GONE,
                  "service does not exist (anymore)");
            }
          } catch (std::exception &exception) {
            LOG_ERR("cannot execute service "
                    << request->GetServiceName()
                    << " called from remote peer via web-socket: internal "
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

          if (_this->m_web_socket_peer.expired()) {
            LOG_ERR("cannot send service response for request "
                    << request->GetTransactionId()
                    << " because web-socket peer has shut down")
          } else {
            auto sending_progress = _this->m_web_socket_peer.lock()->Send(
                connection_info.GetHostname(), response_message);
            context->GetJobManager()->WaitForCompletion(sending_progress);

            try {
              sending_progress.get();
            } catch (std::exception &exception) {
              LOG_ERR("error while sending response for service request "
                      << request->GetTransactionId() << " for service "
                      << request->GetServiceName()
                      << " due to sending error: " << exception.what())
            }
          }

          return JobContinuation::DISPOSE;
        });

    job_manager->KickJob(job);
  } else /* if subsystems are not available */ {
    if (m_subsystems.expired()) {
      LOG_ERR("Cannot process received message because required subsystems are "
              "not available or have been shut down")
    }
  }
}