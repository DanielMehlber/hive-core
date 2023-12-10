#ifndef IWEBSOCKETSERVER_H
#define IWEBSOCKETSERVER_H

#include "IPeerMessageConsumer.h"
#include "common/exceptions/ExceptionsBase.h"
#include "properties/PropertyProvider.h"
#include <future>
#include <list>
#include <memory>

namespace networking::websockets {

DECLARE_EXCEPTION(DuplicateConsumerTypeException);
DECLARE_EXCEPTION(PeerSetupException);

/**
 * @brief A generic interface declaring a web-socket peer that can accept
 * connections,
 */
class IPeer {
public:
  /**
   * @brief Adds a consumer for a specific type of arriving events to the
   * register.
   * @attention Every type of message can only have a single consumer at a time,
   * so the type must be unique.
   * @param consumer consumer of specific type to add to the register
   * @throws DuplicateConsumerTypeException if there is already a consumer for
   * that type registered.
   */
  virtual void
  AddMessageConsumer(std::weak_ptr<IPeerMessageConsumer> consumer) = 0;

  /**
   * @brief Tries to retrieve a valid consumer for the given message type (if
   * one is registered and has not expired yet)
   * @param type_name type name of the events that the consumer processes
   * @return a consumer if one has been found for the given type
   */
  virtual std::list<SharedPeerMessageConsumer>
  GetConsumersOfMessageType(const std::string &type_name) noexcept = 0;

  /**
   * @brief Sends some message to some uri asynchronously
   * @attention If there is no existing connection with the socket at that uri,
   * this operation will try to establish the connection.
   * @param uri identifier of the socket that should receive the message.
   * @param message message that should be sent
   * @return a future indicating the completion of the sending process or
   * contains exception if it failed.
   */
  virtual std::future<void> Send(const std::string &uri,
                                 SharedWebSocketMessage message) = 0;

  /**
   * @brief Sends some message to all currently connected peers asynchronously
   * @param message message that will be broadcast
   * @return a future representing the broadcast progress and holding the count
   * of recipients.
   */
  virtual std::future<size_t>
  Broadcast(const SharedWebSocketMessage &message) = 0;

  /**
   * Counts active and usable connections
   * @return count of active and usable connections
   */
  virtual size_t GetActiveConnectionCount() const = 0;

  /**
   * @brief Establishes connection with another socket server
   * @param uri where the other socket server listenes for new connections
   * @return a future indicating success or failure of the connecting process.
   * @note This uri is unique and will be treated like a key that identifies the
   * connection. If there already exists a connection to this uri, a new one
   * will not be established.
   */
  virtual std::future<void>
  EstablishConnectionTo(const std::string &uri) noexcept = 0;

  /**
   * @brief Closes a connection to a socket located at the uri (if the
   * connection still exists)
   * @param uri identifier of the connection that should be closed.
   * @note connections that are not established will be ignored.
   */
  virtual void CloseConnectionTo(const std::string &uri) noexcept = 0;

  /**
   * @brief Checks if this peer is currently connected to an endpoint with this
   * url
   * @param uri identifier of connection
   * @return true, if there is an established connection to this host
   */
  virtual bool HasConnectionTo(const std::string &uri) const noexcept = 0;
};

typedef std::shared_ptr<IPeer> SharedWebSocketPeer;

} // namespace networking::websockets

#endif /* IWEBSOCKETSERVER_H */
