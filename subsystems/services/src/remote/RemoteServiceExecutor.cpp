#include "services/executor/impl/RemoteServiceExecutor.h"
#include "common/uuid/UuidGenerator.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace hive::services::impl;
using namespace hive::services;
using namespace hive::jobsystem;

RemoteServiceExecutor::RemoteServiceExecutor(
    std::string service_name,
    common::memory::Reference<IMessageEndpoint> endpoint,
    ConnectionInfo remote_host_info, std::string id,
    std::weak_ptr<RemoteServiceResponseConsumer> response_consumer)
    : m_endpoint(std::move(endpoint)),
      m_remote_host_info(std::move(remote_host_info)),
      m_service_name(std::move(service_name)),
      m_response_consumer(std::move(response_consumer)), m_id(std::move(id)) {}

bool RemoteServiceExecutor::IsCallable() {
  if (auto maybe_endpoint = m_endpoint.TryBorrow()) {
    return maybe_endpoint.value()->HasConnectionTo(
        m_remote_host_info.endpoint_id);
  } else {
    return false;
  }
}

std::future<SharedServiceResponse> RemoteServiceExecutor::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<jobsystem::JobManager> job_manager, bool async) {

  auto promise = std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> future = promise->get_future();
  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [_this = shared_from_this(), request, promise](JobContext *context) {
        if (_this->IsCallable()) {
          LOG_DEBUG("calling remote web-socket service '"
                    << request->GetServiceName() << "' at node "
                    << _this->m_remote_host_info.endpoint_id)

          if (!_this->m_response_consumer.expired()) {
            /* pass the promise (resolving the service request) to the response
            consumer. When its response arrives, the request promise will be
            resolved. */
            _this->m_response_consumer.lock()->AddResponsePromise(
                request->GetTransactionId(), _this->m_remote_host_info,
                std::move(*promise));
          } else {
            LOG_ERR("cannot receive response to service request "
                    << request->GetTransactionId() << " of service "
                    << request->GetServiceName()
                    << " because response consumer has been destroyed")
            auto exception = BUILD_EXCEPTION(
                CallFailedException, "called remote web-socket service, but "
                                     "cannot receive response because "
                                     "its consumer has been destroyed");
            promise->set_exception(std::make_exception_ptr(exception));
          }

          SharedMessage message =
              RemoteServiceMessagesConverter::FromServiceRequest(*request);

          std::future<void> sending_progress = _this->m_endpoint.Borrow()->Send(
              _this->m_remote_host_info.endpoint_id, message);

          // wait until request has been sent and register promise for
          // resolution
          context->GetJobManager()->WaitForCompletion(sending_progress);

          try {
            // check if sending was successful (or threw exception)
            sending_progress.get();
          } catch (std::exception &sending_exception) {
            auto maybe_promise =
                _this->m_response_consumer.lock()->RemoveResponsePromise(
                    request->GetTransactionId());

            if (maybe_promise.has_value()) {
              auto response_promise = std::move(maybe_promise.value());
              LOG_ERR("failed to call remote service '"
                      << request->GetServiceName() << "' of node "
                      << _this->m_remote_host_info.endpoint_id << " at "
                      << _this->m_remote_host_info.hostname << ": "
                      << sending_exception.what())
              auto except = BUILD_EXCEPTION(
                  CallFailedException,
                  "failed to call remote service '"
                      << request->GetServiceName() << "' of node "
                      << _this->m_remote_host_info.endpoint_id << " at "
                      << _this->m_remote_host_info.hostname << ": "
                      << sending_exception.what());

              response_promise.set_exception(std::make_exception_ptr(except));
            }
          }
        } else {
          LOG_ERR("cannot call remote web-socket service "
                  << request->GetServiceName() << " of node "
                  << _this->m_remote_host_info.endpoint_id << " at "
                  << _this->m_remote_host_info.hostname)
          auto exception = BUILD_EXCEPTION(
              CallFailedException,
              "cannot call remote web-socket service "
                  << request->GetServiceName() << " of node "
                  << _this->m_remote_host_info.endpoint_id << " at "
                  << _this->m_remote_host_info.hostname);
          promise->set_exception(std::make_exception_ptr(exception));
        }
        return JobContinuation::DISPOSE;
      },
      "remote-service-call-" + request->GetTransactionId(), MAIN, async);

  job_manager->KickJob(job);
  return future;
}

std::string RemoteServiceExecutor::GetServiceName() { return m_service_name; }
