#pragma once

#include <map>
#include <memory>
#include <string>

namespace hive::common::config {

/**
 * Contains configuration data for subsystems to apply at their startup.
 */
class Configuration {
protected:
  /** Contains configuration as key-value pairs */
  std::map<std::string, std::string> m_values;

public:
  /**
   * Serializes some value to string and assigns it to a configuration
   * name that can be queried later by subsystems.
   * @tparam T type of value (must be serializable)
   * @param name configuration name
   * @param value value assigned to configuration name
   * @note already set values will be overwritten
   */
  template <typename T = std::string>
  void Set(const std::string &name, T value);

  /**
   * Retrieve a configuration name's value as string. Returns an
   * alternative value if it has not been set.
   * @param name name of configuration value
   * @param alternative alternative value is there is no configuration value for
   * requested name
   * @return value or alternative as string
   */
  std::string Get(const std::string &name, std::string alternative);

  /**
   * Retrieve a configuration name's string value as int by parsing it.
   * Returns an alternative int value if it has not been set.
   * @param name name of configuration value
   * @param alternative alternative value is there is no configuration value for
   * requested name
   * @return value or alternative as int
   * @throw std::invalid_argument if no conversion could be performed.
   */
  int GetAsInt(const std::string &name, int alternative);

  /**
   * Retrieve a configuration name's string value as float by parsing it.
   * Returns an alternative float value if it has not been set.
   * @param name name of configuration value
   * @param alternative alternative value is there is no configuration value for
   * requested name
   * @return value or alternative as float
   * @throw std::invalid_argument if no conversion could be performed.
   */
  float GetAsFloat(const std::string &name, float alternative);

  /**
   * Retrieve a configuration name's string value as boolean by parsing
   * it. If the value is 'true' or '1', the boolean value true is returned.
   * Otherwise the boolean value false is returned. Returns an alternative
   * boolean value if it has not been set.
   * @param name name of configuration value
   * @param alternative alternative value is there is no configuration value for
   * requested name
   * @return value or alternative as boolean
   */
  bool GetBool(const std::string &name, bool alternative);

  /**
   * Try to find a value for the requested configuration name and return true if
   * it has been found.
   * @param name name of requested configuration name
   * @return true if configuration name has value
   */
  bool Contains(const std::string &name);
};

inline bool Configuration::Contains(const std::string &name) {
  return m_values.contains(name);
}

inline std::string Configuration::Get(const std::string &name,
                                      std::string alternative) {
  if (Contains(name)) {
    return m_values.at(name);
  } else {
    return alternative;
  }
}

inline int Configuration::GetAsInt(const std::string &name, int alternative) {
  if (Contains(name)) {
    return std::stoi(m_values.at(name));
  } else {
    return alternative;
  }
}

inline float Configuration::GetAsFloat(const std::string &name,
                                       float alternative) {
  if (Contains(name)) {
    return std::stof(m_values.at(name));
  } else {
    return alternative;
  }
}

inline bool Configuration::GetBool(const std::string &name, bool alternative) {
  if (Contains(name)) {
    const auto &value = m_values.at(name);
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

} // namespace hive::common::config