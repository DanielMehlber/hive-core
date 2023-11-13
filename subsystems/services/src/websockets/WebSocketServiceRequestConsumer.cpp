#include "services/registry/impl/websockets/WebSocketServiceRequestConsumer.h"
#include "services/registry/impl/websockets/WebSocketServiceMessagesConverter.h"

using namespace services::impl;

WebSocketServiceRequestConsumer::WebSocketServiceRequestConsumer(
    const common::subsystems::SharedSubsystemManager &subsystems,
    WebSocketServiceRequestConsumer::query_func_type query_func,
    const SharedWebSocketPeer &web_socket_peer)
    : m_subsystems(subsystems), m_service_query_func(std::move(query_func)),
      m_web_socket_peer(web_socket_peer) {}

void WebSocketServiceRequestConsumer::ProcessReceivedMessage(
    SharedWebSocketMessage received_message,
    WebSocketConnectionInfo connection_info) noexcept {

  if (m_subsystems.expired()) {
    LOG_ERR("Cannot process received message because subsystems have already "
            "shut down")
    return;
  }

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();

  auto opt_request =
      WebSocketServiceMessagesConverter::ToServiceRequest(received_message);

  if (!opt_request.has_value()) {
    LOG_WARN("received invalid service request. Dropped.")
    return;
  }

  SharedServiceRequest request = opt_request.value();
  LOG_DEBUG("received service request for service "
            << request->GetServiceName())

  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [_this = std::static_pointer_cast<WebSocketServiceRequestConsumer>(
           shared_from_this()),
       request, connection_info](jobsystem::JobContext *context) {
        auto job_manager = _this->m_subsystems.lock()
                               ->RequireSubsystem<jobsystem::JobManager>();

        auto fut_opt_service_caller =
            _this->m_service_query_func(request->GetServiceName(), true);

        context->GetJobManager()->WaitForCompletion(fut_opt_service_caller);

        SharedServiceResponse response;

        try {
          auto opt_service_caller = fut_opt_service_caller.get();
          if (opt_service_caller.has_value()) {
            auto service_caller = opt_service_caller.value();

            // call service locally
            std::future<SharedServiceResponse> fut_response =
                service_caller->Call(request, job_manager, true);

            context->GetJobManager()->WaitForCompletion(fut_response);
            response = fut_response.get();

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
              ServiceResponseStatus::INTERNAL_ERROR, "internal error occurred");
        }

        SharedWebSocketMessage response_message =
            WebSocketServiceMessagesConverter::FromServiceResponse(
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
}