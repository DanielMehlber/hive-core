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
/**
 * Constructs various objects of the networking module
 */
class NetworkingFactory {
public:
  /**
   * @brief Create new web-socket peer implementation
   * @tparam ServerImpl Implementation type of IWebSocketPeer interface
   * @tparam Args Arguments types
   * @param args Arguments that will be passed to the implementation's
   * constructor
   * @return a shared web-socket peer implementation
   */
  template <typename ServerImpl = DEFAULT_WEBSOCKET_PEER_IMPL, typename... Args>
  static SharedWebSocketPeer CreateWebSocketPeer(Args... args);
};

template <typename ServerImpl, typename... Args>
inline SharedWebSocketPeer
NetworkingFactory::CreateWebSocketPeer(Args... args) {
  return std::static_pointer_cast<IWebSocketPeer>(
      std::make_shared<ServerImpl>(args...));
}

} // namespace networking

#endif /* NETWORKINGFACTORY_H */
