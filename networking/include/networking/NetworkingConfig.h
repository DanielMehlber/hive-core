#ifndef NETWORKINGCONFIGURATION_H
#define NETWORKINGCONFIGURATION_H

#include "networking/websockets/WebSocketConfiguration.h"

namespace networking {
class NetworkingConfig {
protected:
  /**
   * @brief Should the websocket server be initialized at startup?
   */
  bool m_init_websocket_server{true};
  websockets::WebSocketConfiguration m_web_socket_config;

public:
  bool ShoudInitWebSocketServer() noexcept;
  void SetInitWebSocketServer(bool value) noexcept;

  websockets::WebSocketConfiguration &GetWebSocketConfig() noexcept;
};

inline void NetworkingConfig::SetInitWebSocketServer(bool value) noexcept {
  m_init_websocket_server = value;
}

inline bool NetworkingConfig::ShoudInitWebSocketServer() noexcept {
  return m_init_websocket_server;
}

inline websockets::WebSocketConfiguration &
NetworkingConfig::GetWebSocketConfig() noexcept {
  return m_web_socket_config;
}

} // namespace networking

#endif /* NETWORKINGCONFIGURATION_H */
