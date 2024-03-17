#ifndef WEBSOCKETCONNECTIONINFO_H
#define WEBSOCKETCONNECTIONINFO_H

#include <string>

namespace networking::messaging {

class ConnectionInfo {
protected:
  std::string m_hostname;

public:
  void SetHostname(const std::string &hostname);
  std::string GetHostname() const;
};

inline void ConnectionInfo::SetHostname(const std::string &hostname) {
  m_hostname = hostname;
}

inline std::string ConnectionInfo::GetHostname() const { return m_hostname; }

} // namespace networking::messaging

#endif // WEBSOCKETCONNECTIONINFO_H
