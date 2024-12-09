#include "services/executor/impl/RemoteServiceExecutor.h"
#include "logging/LogManager.h"
#include "services/registry/impl/remote/RemoteExceptions.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace hive::services::impl;
using namespace hive::services;
using namespace hive::jobsystem;
using namespace hive::networking::messaging;
using namespace hive::networking;

RemoteServiceExecutor::RemoteServiceExecutor(
    std::string service_name,
    common::memory::Reference<NetworkingManager> networking_mgr,
    ConnectionInfo remote_host_info, std::string id,
    std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
    size_t capacity)
    : m_networking_manager(std::move(networking_mgr)),
      m_remote_host_info(std::move(remote_host_info)),
      m_service_name(std::move(service_name)),
      m_response_consumer(std::move(response_consumer)), m_id(std::move(id)),
      m_capacity(capacity) {}

bool RemoteServiceExecutor::IsCallable() {
  if (auto maybe_networking_manager = m_networking_manager.TryBorrow()) {
    auto maybe_connection =
        maybe_networking_manager.value()->GetSomeMessageEndpointConnectedTo(
            m_remote_host_info.remote_endpoint_id);
    return maybe_connection.has_value();
  }
  return false;
}

std::string get_exception_string(const std::exception_ptr &ptr) {
  try {
    std::rethrow_exception(ptr);
  } catch (const std::exception &e) {
    return e.what();
  } catch (...) {
    return "unknown exception";
  }
}

std::future<SharedServiceResponse> RemoteServiceExecutor::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<JobManager> job_manager, bool async) {

  DEBUG_ASSERT(request != nullptr, "request should not be null")
  DEBUG_ASSERT(!request->GetServiceName().empty(),
               "service name should not be empty")
  DEBUG_ASSERT(!request->GetTransactionId().empty(),
               "service transaction id should not be empty")
  DEBUG_ASSERT(
      !request->IsCurrentlyProcessed(),
      "double using requests is prohibited while it is still pending because "
      "of the identical transaction id. Duplicate the requests instead before "
      "second use.")

  auto promise = std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> future = promise->get_future();

  SharedJob job = std::make_shared<Job>(
      [executor = shared_from_this(), request,
       promise](JobContext *context) mutable {
        if (executor->IsCallable()) {
          LOG_DEBUG("calling remote web-socket service '"
                    << request->GetServiceName() << "' at node "
                    << executor->m_remote_host_info.remote_endpoint_id)

          if (!executor->m_response_consumer.expired()) {
            /* pass the promise (resolving the service request) to the response
            consumer. When its response arrives, the request promise will be
            resolved. */
            DEBUG_ASSERT(
                !request->IsCurrentlyProcessed(),
                "double processing the same request while it's still pending "
                "is prohibited. Duplicate the request for that purpose or wait "
                "until it has been resolved once.")
            request->MarkAsCurrentlyProcessed(true);
            executor->m_response_consumer.lock()->AddResponsePromise(
                request, executor->m_remote_host_info, promise);
          } else {
            LOG_ERR("cannot receive response to service request "
                    << request->GetTransactionId() << " of service "
                    << request->GetServiceName()
                    << " because response consumer has been destroyed")
            auto exception = BUILD_EXCEPTION(
                CallFailedException, "called remote web-socket service, but "
                                     "cannot receive response because "
                                     "its consumer has been destroyed");
            request->MarkAsCurrentlyProcessed(false);
            promise->set_exception(std::make_exception_ptr(exception));
          }

          SharedMessage message =
              RemoteServiceMessagesConverter::FromServiceRequest(*request);

          auto maybe_connected_endpoint =
              executor->m_networking_manager.Borrow()
                  ->GetSomeMessageEndpointConnectedTo(
                      executor->m_remote_host_info.remote_endpoint_id);

          if (!maybe_connected_endpoint.has_value()) {
            LOG_ERR("cannot call remote web-socket service "
                    << request->GetServiceName() << " of node "
                    << executor->m_remote_host_info.remote_endpoint_id << " at "
                    << executor->m_remote_host_info.remote_url
                    << " because no endpoint is connected to remote host")

            auto exception = BUILD_EXCEPTION(
                CallFailedException,
                "cannot call remote web-socket service "
                    << request->GetServiceName() << " of node "
                    << executor->m_remote_host_info.remote_endpoint_id << " at "
                    << executor->m_remote_host_info.remote_url
                    << " because no endpoint is connected to remote host");

            request->MarkAsCurrentlyProcessed(false);
            promise->set_exception(std::make_exception_ptr(exception));
            return JobContinuation::DISPOSE;
          }

          auto connected_endpoint = maybe_connected_endpoint.value();

          std::future<void> sending_progress = connected_endpoint->Send(
              executor->m_remote_host_info.remote_endpoint_id, message);

          // wait until request has been sent and register promise for
          // resolution
          context->GetJobManager()->Await(sending_progress);

          try {
            // check if sending was successful (or threw exception)
            sending_progress.get();
          } catch (...) {
            std::string what = get_exception_string(std::current_exception());
            executor->m_response_consumer.lock()->RemoveResponsePromise(
                request->GetTransactionId());

            LOG_ERR("failed to call remote service '"
                    << request->GetServiceName() << "' of node "
                    << executor->m_remote_host_info.remote_endpoint_id << " at "
                    << executor->m_remote_host_info.remote_url << ": " << what)
            auto except = BUILD_EXCEPTION(
                CallFailedException,
                "failed to call remote service '"
                    << request->GetServiceName() << "' of node "
                    << executor->m_remote_host_info.remote_endpoint_id << " at "
                    << executor->m_remote_host_info.remote_url << ": " << what);

            request->MarkAsCurrentlyProcessed(false);
            promise->set_exception(std::make_exception_ptr(except));
          }
        } else {
          LOG_ERR("cannot call remote web-socket service "
                  << request->GetServiceName() << " of node "
                  << executor->m_remote_host_info.remote_endpoint_id << " at "
                  << executor->m_remote_host_info.remote_url)
          auto exception = BUILD_EXCEPTION(
              CallFailedException,
              "cannot call remote web-socket service "
                  << request->GetServiceName() << " of node "
                  << executor->m_remote_host_info.remote_endpoint_id << " at "
                  << executor->m_remote_host_info.remote_url);

          request->MarkAsCurrentlyProcessed(false);
          promise->set_exception(std::make_exception_ptr(exception));
        }
        return JobContinuation::DISPOSE;
      },
      "remote-service-call-" + request->GetTransactionId(), MAIN, async);

  job_manager->KickJob(job);
  return future;
}
