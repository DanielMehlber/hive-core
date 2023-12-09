#ifndef SIMULATION_FRAMEWORK_CONFIGURATION_H
#define SIMULATION_FRAMEWORK_CONFIGURATION_H

#include <map>
#include <memory>
#include <utility>

namespace common::config {

class Configuration {
protected:
  std::map<std::string, std::string> m_values;

public:
  template <typename T = std::string>
  void Set(const std::string &name, T value);

  std::string Get(const std::string &name, std::string alternative);
  int GetAsInt(const std::string &name, int alternative);
  float GetAsFloat(const std::string &name, float alternative);
  bool GetBool(const std::string &name, bool alternative);
};

inline std::string Configuration::Get(const std::string &name,
                                      std::string alternative) {
  if (m_values.contains(name)) {
    return m_values.at(name);
  } else {
    return alternative;
  }
}

inline int Configuration::GetAsInt(const std::string &name, int alternative) {
  if (m_values.contains(name)) {
    return std::stoi(m_values.at(name));
  } else {
    return alternative;
  }
}

inline float Configuration::GetAsFloat(const std::string &name,
                                       float alternative) {
  if (m_values.contains(name)) {
    return std::stof(m_values.at(name));
  } else {
    return alternative;
  }
}

inline bool Configuration::GetBool(const std::string &name, bool alternative) {
  if (m_values.contains(name)) {
    auto value = m_values.at(name);
    if (value == "true" || value == "1") {
      return true;
    } else {
      return false;
    }
  } else {
    return alternative;
  }
}

template <typename T>
inline void Configuration::Set(const std::string &name, T value) {
  m_values[name] = std::to_string(value);
}

template <>
inline void Configuration::Set<std::string>(const std::string &name,
                                            std::string value) {
  m_values[name] = std::move(value);
}

typedef std::shared_ptr<Configuration> SharedConfiguration;

} // namespace common::config

#endif // SIMULATION_FRAMEWORK_CONFIGURATION_H
