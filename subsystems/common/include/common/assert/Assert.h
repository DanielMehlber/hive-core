#ifndef SIMULATION_FRAMEWORK_ASSERT_H
#define SIMULATION_FRAMEWORK_ASSERT_H

#include "common/exceptions/ExceptionsBase.h"

DECLARE_EXCEPTION(AssertionFailedException);

#ifndef NDEBUG
#define ASSERT(x, message)                                                     \
  {                                                                            \
    if (!(x)) {                                                                \
      THROW_EXCEPTION(AssertionFailedException,                                \
                      "assertion " << #x << " failed at " << __FILE__ << ":"   \
                                   << __LINE__ << ": " << message);            \
    }                                                                          \
  }
#else
#define ASSERT(x, message)
#endif

#endif // SIMULATION_FRAMEWORK_ASSERT_H
