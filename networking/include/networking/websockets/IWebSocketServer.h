#ifndef IWEBSOCKETSERVER_H
#define IWEBSOCKETSERVER_H

#include "IWebSocketMessageConsumer.h"
#include "WebSocketConfiguration.h"
#include "common/exceptions/ExceptionsBase.h"
#include <future>
#include <memory>

namespace networking::websockets {

DECLARE_EXCEPTION(DuplicateConsumerTypeException);

class IWebSocketServer {
public:
  /**
   * @brief Initializes the web socket server
   * @param configuration will be applied to the launched server
   */
  virtual void Init(const WebSocketConfiguration &configuration) = 0;

  /**
   * @brief Shuts the web socket server down and closes all connections
   */
  virtual void Shutdown() = 0;

  /**
   * @brief Adds a consumer for a specific type of arriving messages to the
   * register.
   * @attention Every type of message can only have a single consumer at a time,
   * so the type must be unique.
   * @param consumer consumer of specific type to add to the register
   * @throws DuplicateConsumerTypeException if there is already a consumer for
   * that type registered.
   */
  virtual void
  AddConsumer(std::weak_ptr<IWebSocketMessageConsumer> consumer) = 0;

  /**
   * @brief Tries to retrieve a valid consumer for the given message type (if
   * one is registered and has not expired yet)
   * @param type_name type name of the messages that the consumer processes
   * @return a consumer if one has been found for the given type
   */
  virtual std::optional<SharedWebSocketMessageConsumer>
  GetConsumerForType(const std::string &type_name) noexcept = 0;

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
                                 SharedWebSocketMessage message) noexcept = 0;

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
};

typedef std::shared_ptr<IWebSocketServer> SharedWebSocketServer;

} // namespace networking::websockets

#endif /* IWEBSOCKETSERVER_H */
