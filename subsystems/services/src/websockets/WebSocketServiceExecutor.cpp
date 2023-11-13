#include <utility>

#include "common/uuid/UuidGenerator.h"
#include "services/executor/impl/WebSocketServiceExecutor.h"
#include "services/registry/impl/websockets/WebSocketServiceMessagesConverter.h"

using namespace services::impl;
using namespace services;
using namespace jobsystem;

WebSocketServiceExecutor::WebSocketServiceExecutor(
    std::string service_name, std::weak_ptr<IWebSocketPeer> peer,
    std::string remote_host_name,
    std::weak_ptr<WebSocketServiceResponseConsumer> response_consumer)
    : m_web_socket_peer(std::move(peer)),
      m_remote_host_name(std::move(remote_host_name)),
      m_service_name(std::move(service_name)),
      m_response_consumer(std::move(response_consumer)) {}

bool WebSocketServiceExecutor::IsCallable() {
  if (m_web_socket_peer.expired()) {
    return false;
  }

  return m_web_socket_peer.lock()->HasConnectionTo(m_remote_host_name);
}

std::future<SharedServiceResponse>
WebSocketServiceExecutor::Call(SharedServiceRequest request,
                               jobsystem::SharedJobManager job_manager) {

  auto promise = std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> future = promise->get_future();
  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [_this = shared_from_this(), request, promise](JobContext *context) {
        if (_this->IsCallable()) {
          LOG_DEBUG("calling remote web-socket service '"
                    << request->GetServiceName() << "' at endpoint "
                    << _this->m_remote_host_name);

          if (!_this->m_response_consumer.expired()) {
            _this->m_response_consumer.lock()->AddResponsePromise(
                request->GetTransactionId(), std::move(*promise));
          } else {
            LOG_ERR("cannot receive response to service request "
                    << request->GetTransactionId() << " of service "
                    << request->GetServiceName()
                    << " because response consumer has been destroyed");
            auto exception = BUILD_EXCEPTION(
                CallFailedException, "called remote web-socket service, but "
                                     "cannot receive response because "
                                     "its consumer has been destroyed");
            promise->set_exception(std::make_exception_ptr(exception));
          }

          SharedWebSocketMessage message =
              WebSocketServiceMessagesConverter::FromServiceRequest(*request);

          std::future<void> sending_future =
              _this->m_web_socket_peer.lock()->Send(_this->m_remote_host_name,
                                                    message);

          // wait until request has been sent and register promise for
          // resolution
          context->GetJobManager()->WaitForCompletion(sending_future);

        } else {
          LOG_ERR("cannot call remote web-socket service "
                  << request->GetServiceName() << " at "
                  << _this->m_remote_host_name);
          auto exception = BUILD_EXCEPTION(
              CallFailedException, "cannot call remote web-socket service "
                                       << request->GetServiceName() << " at "
                                       << _this->m_remote_host_name);
          promise->set_exception(std::make_exception_ptr(exception));
        }
        return JobContinuation::DISPOSE;
      });

  job_manager->KickJob(job);
  return future;
}

std::string WebSocketServiceExecutor::GetServiceName() {
  return m_service_name;
}
