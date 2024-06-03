#pragma once

#ifndef NDEBUG
#include <iostream>
#define DEBUG_ASSERT(x, message)                                               \
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
