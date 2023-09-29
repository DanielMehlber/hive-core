#ifndef WEBSOCKETCONFIGURATION_H
#define WEBSOCKETCONFIGURATION_H

#include <string>

namespace networking::websockets {
class WebSocketConfiguration {
private:
  size_t m_port{9000};
  bool m_use_tls{true};

public:
  size_t GetPort() const noexcept;
  void SetPort(size_t port) noexcept;

  bool GetUseTls() const noexcept;
  void SetUseTls(bool use_tls) noexcept;
};

inline size_t WebSocketConfiguration::GetPort() const noexcept {
  return m_port;
}

inline void WebSocketConfiguration::SetPort(size_t port) noexcept {
  m_port = port;
}

inline bool WebSocketConfiguration::GetUseTls() const noexcept {
  return m_use_tls;
}

inline void WebSocketConfiguration::SetUseTls(bool use_tls) noexcept {
  m_use_tls = use_tls;
}

} // namespace networking::websockets

#endif /* WEBSOCKETCONFIGURATION_H */
