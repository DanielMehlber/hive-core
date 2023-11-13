#ifndef WEBSOCKETCONNECTIONINFO_H
#define WEBSOCKETCONNECTIONINFO_H

#include <string>

namespace networking::websockets {

class WebSocketConnectionInfo {
protected:
  std::string m_hostname;

public:
  void SetHostname(const std::string &hostname);
  std::string GetHostname() const;
};

inline void WebSocketConnectionInfo::SetHostname(const std::string &hostname) {
  m_hostname = hostname;
}

inline std::string WebSocketConnectionInfo::GetHostname() const {
  return m_hostname;
}

} // namespace networking::websockets

#endif // WEBSOCKETCONNECTIONINFO_H
