#ifndef EXCEPTIONSBASE_H
#define EXCEPTIONSBASE_H

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>
#include <sstream>

class ExceptionBase : public std::exception {
protected:
  const boost::stacktrace::stacktrace m_stacktrace_print;
  const std::string m_message;

public:
  ExceptionBase(const std::stringstream &message,
                const boost::stacktrace::stacktrace &stacktrace)
      : m_stacktrace_print{stacktrace}, m_message{message.str()} {}

  virtual const char *what() const noexcept override {
    return m_message.c_str();
  }

  virtual const char *get_type() const noexcept = 0;

  const std::string to_string() {
    std::stringstream __ss;
    __ss << "Caught " << get_type() << ": " << m_message << std::endl;
    __ss << "Stacktrace: " << std::endl << m_stacktrace_print << std::endl;
    return __ss.str();
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
  };

#define THROW_EXCEPTION(ExceptionType, message)                                \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << message;                                                           \
    throw ExceptionType(__ss, boost::stacktrace::stacktrace());                \
  };

#endif /* EXCEPTIONSBASE_H */
