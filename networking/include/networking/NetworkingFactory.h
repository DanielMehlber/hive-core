#ifndef NETWORKINGFACTORY_H
#define NETWORKINGFACTORY_H

#include "websockets/IWebSocketServer.h"
#include "websockets/impl/WebSocketppServer.h"
#include <memory>

using namespace networking::websockets;

#ifndef DEFAULT_WEBSOCKET_SERVER_IMPL
#define DEFAULT_WEBSOCKET_SERVER_IMPL networking::websockets::WebSocketppServer
#endif

namespace networking {
class NetworkingFactory {
public:
  template <typename ServerImpl = DEFAULT_WEBSOCKET_SERVER_IMPL,
            typename... Args>
  static SharedWebSocketServer CreateWebSocketServer(Args... args);
};

template <typename ServerImpl = DEFAULT_WEBSOCKET_SERVER_IMPL, typename... Args>
inline SharedWebSocketServer
NetworkingFactory::CreateWebSocketServer(Args... args) {
  return std::static_pointer_cast<IWebSocketServer>(
      std::make_shared<ServerImpl>(args...));
}

} // namespace networking

#endif /* NETWORKINGFACTORY_H */
