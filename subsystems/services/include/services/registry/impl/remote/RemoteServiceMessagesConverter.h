#pragma once

#include "networking/messaging/Message.h"
#include "services/ServiceRequest.h"
#include "services/ServiceResponse.h"

using namespace hive::networking::messaging;

namespace hive::services::impl {

/**
 * Converts web-socket message objects to service objects
 */
class RemoteServiceMessagesConverter {
public:
  /**
   * Consumes and converts a received web-socket message to a service response
   * object
   * @attention Consumes the received message object because attributes are
   * moved, not copied (to avoid possibly copying heavy blobs)
   * @param message message that will be converted and consumed
   * @return service response object (or nothing if message was invalid)
   */
  static std::optional<SharedServiceResponse>
  ToServiceResponse(Message &&message);

  /**
   * Consumes and converts a response object to a web-socket message.
   * @attention Consumes the response object because results are moved, not
   * copied (to avoid possibly copying heavy blobs)
   * @param response response object that will be moved and converted
   * @return a web-socket message
   */
  static SharedMessage FromServiceResponse(ServiceResponse &&response);

  /**
   * Converts a received web-socket message to a service request object
   * @param message web-socket message
   * @return service request object
   */
  static std::optional<SharedServiceRequest>
  ToServiceRequest(const SharedMessage &message);

  /**
   * Consumes and Converts a request object to a web-socket message.
   * @param request
   * @return web socket message containing request information
   */
  static SharedMessage
  FromServiceRequest(const services::ServiceRequest &request);
};
} // namespace hive::services::impl
