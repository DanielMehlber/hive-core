#ifndef NETWORKINGFACTORY_H
#define NETWORKINGFACTORY_H

#include "messaging/IMessageEndpoint.h"
#include "messaging/impl/websockets/boost/BoostWebSocketEndpoint.h"
#include <memory>

using namespace networking::websockets;

#ifndef DEFAULT_WEBSOCKET_PEER_IMPL
#define DEFAULT_WEBSOCKET_PEER_IMPL                                            \
  networking::websockets::BoostWebSocketEndpoint
#endif

namespace networking {
/**
 * Constructs various objects of the networking module
 */
class NetworkingFactory {
public:
  /**
   * Create new web-socket peer implementation
   * @tparam ServerImpl Implementation type of IPeer interface
   * @tparam Args Arguments types
   * @param args Arguments that will be passed to the implementation's
   * constructor
   * @return a shared web-socket peer implementation
   */
  template <typename ServerImpl = DEFAULT_WEBSOCKET_PEER_IMPL, typename... Args>
  static SharedMessageEndpoint CreateNetworkingPeer(Args... args);
};

template <typename ServerImpl, typename... Args>
inline SharedMessageEndpoint
NetworkingFactory::CreateNetworkingPeer(Args... args) {
  return std::static_pointer_cast<IMessageEndpoint>(
      std::make_shared<ServerImpl>(args...));
}

} // namespace networking

#endif /* NETWORKINGFACTORY_H */
