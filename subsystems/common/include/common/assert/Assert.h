#ifndef SIMULATION_FRAMEWORK_ASSERT_H
#define SIMULATION_FRAMEWORK_ASSERT_H

#include "common/exceptions/ExceptionsBase.h"

#ifndef NDEBUG
#include <iostream>
#define ASSERT(x, message)                                                     \
  {                                                                            \
    try {                                                                      \
      if (!(x)) {                                                              \
        std::cerr << "assertion " << #x << " failed at " << __FILE__ << ":"    \
                  << __LINE__ << ": " << message << std::endl;                 \
        std::abort();                                                          \
      }                                                                        \
    } catch (std::exception & exception) {                                     \
      std::cerr << "assertion " << #x                                          \
                << "threw an exception: " << exception.what() << std::endl;    \
      std::abort();                                                            \
    }                                                                          \
  }
#else
#define ASSERT(x, message)
#endif

#endif // SIMULATION_FRAMEWORK_ASSERT_H
