#ifndef NETWORKINGFACTORY_H
#define NETWORKINGFACTORY_H

#include "websockets/IWebSocketPeer.h"
#include "websockets/impl/boost/BoostWebSocketPeer.h"
#include <memory>

using namespace networking::websockets;

#ifndef DEFAULT_WEBSOCKET_PEER_IMPL
#define DEFAULT_WEBSOCKET_PEER_IMPL networking::websockets::BoostWebSocketPeer
#endif

namespace networking {
class NetworkingFactory {
public:
  template <typename ServerImpl = DEFAULT_WEBSOCKET_PEER_IMPL, typename... Args>
  static SharedWebSocketPeer CreateWebSocketPeer(Args... args);
};

template <typename ServerImpl = DEFAULT_WEBSOCKET_PEER_IMPL, typename... Args>
inline SharedWebSocketPeer
NetworkingFactory::CreateWebSocketPeer(Args... args) {
  return std::static_pointer_cast<IWebSocketPeer>(
      std::make_shared<ServerImpl>(args...));
}

} // namespace networking

#endif /* NETWORKINGFACTORY_H */
