#pragma once

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>
#include <sstream>
#include <utility>

class ExceptionBase : public std::exception {
protected:
  const boost::stacktrace::stacktrace m_stacktrace_print;
  const std::string m_message;

public:
  ExceptionBase(const std::stringstream &message,
                boost::stacktrace::stacktrace stacktrace)
      : m_stacktrace_print{std::move(stacktrace)}, m_message{message.str()} {}

  const char *what() const noexcept override { return m_message.c_str(); }

  virtual const char *get_type() const noexcept = 0;

  std::string to_string() {
    std::stringstream _ss;
    _ss << "Caught " << get_type() << ": " << m_message << std::endl;
    _ss << "Stacktrace: " << std::endl << m_stacktrace_print << std::endl;
    return _ss.str();
  }
};

#define DECLARE_EXCEPTION(ExceptionType)                                       \
  class ExceptionType : public ExceptionBase {                                 \
  public:                                                                      \
    ExceptionType(const std::stringstream &message,                            \
                  const boost::stacktrace::stacktrace &stacktrace)             \
        : ExceptionBase(message, stacktrace) {}                                \
    virtual const char *get_type() const noexcept override {                   \
      return #ExceptionType;                                                   \
    }                                                                          \
  }

#define THROW_EXCEPTION(ExceptionType, message)                                \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << message;                                                           \
    throw ExceptionType(__ss, boost::stacktrace::stacktrace());                \
  }

#define BUILD_EXCEPTION(ExceptionType, message)                                \
  [&]() {                                                                      \
    std::stringstream __ss;                                                    \
    __ss << message;                                                           \
    return ExceptionType(__ss, boost::stacktrace::stacktrace());               \
  }()
