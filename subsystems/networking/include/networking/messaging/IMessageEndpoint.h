#pragma once

#include "IMessageConsumer.h"
#include "common/exceptions/ExceptionsBase.h"
#include "properties/PropertyProvider.h"
#include <future>
#include <list>
#include <memory>

namespace hive::networking::messaging {

DECLARE_EXCEPTION(DuplicateConsumerTypeException);
DECLARE_EXCEPTION(EndpointSetupException);

/**
 * A local endpoint that establishes connections to endpoints of other nodes and
 * allows message-oriented communication to them.
 */
class IMessageEndpoint {
public:
  /**
   * Registers a consumer for a certain type of message. Incoming messages of
   * this type will be redirected to the consumer.
   * @attention Every type of message can only have a single consumer at a time,
   * so the type must be unique.
   * @param consumer consumer of specific type to add to the register
   * @throws DuplicateConsumerTypeException if there is already a consumer for
   * that type registered.
   */
  virtual void AddMessageConsumer(std::weak_ptr<IMessageConsumer> consumer) = 0;

  /**
   * Tries to retrieve a valid consumer for the given message type (if
   * one is registered and has not expired yet)
   * @param type_name type name of the events that the consumer processes
   * @return a consumer if one has been found for the given type
   */
  virtual std::list<SharedMessageConsumer>
  GetConsumersOfMessageType(const std::string &type_name) noexcept = 0;

  /**
   * Sends a message to an endpoint associated with the id.
   * @attention If there is no existing connection with the node,
   * this operation will try to establish the connection.
   * @param node_id unique identifier of a node in the cluster. There must be a
   * connection between this endpoint and the node with this id.
   * @param message message that should be sent
   * @return a future indicating the completion of the sending process or
   * contains exception if it failed.
   */
  virtual std::future<void> Send(const std::string &node_id,
                                 SharedMessage message) = 0;

  /**
   * Sends some message to all currently connected peers.
   * @param message message that will be broadcast
   * @return a future representing the broadcast progress and holding the count
   * of recipients.
   */
  virtual std::future<size_t>
  IssueBroadcastAsJob(const SharedMessage &message) = 0;

  /**
   * Counts active and usable connections of this endpoint to others.
   * @return count of active and usable connections
   */
  virtual size_t GetActiveConnectionCount() const = 0;

  /**
   * Establishes connection with another endpoint.
   * @param uri id of the endpoint to connect to. Depending on the
   * underlying implementation, this might be a network address.
   * @return a future indicating success or failure of the connecting process.
   * @note This id is unique and will be treated like a key that identifies the
   * connection. If there already exists a connection to this id, a new one
   * will not be established.
   */
  virtual std::future<ConnectionInfo>
  EstablishConnectionTo(const std::string &uri) noexcept = 0;

  /**
   * Closes a connection to an endpoint (if the connection is still open).
   * @param node_id unique identifier of a node in the cluster.
   * @note connections that are not established will be ignored.
   */
  virtual void CloseConnectionTo(const std::string &node_id) noexcept = 0;

  /**
   * Checks if this endpoint is currently connected to another one with this
   * id.
   * @param node_id unique identifier of a node in the cluster.
   * @return true, if there is an established connection to this host
   */
  virtual bool HasConnectionTo(const std::string &node_id) const noexcept = 0;

  virtual ~IMessageEndpoint() = default;
};

} // namespace hive::networking::messaging
