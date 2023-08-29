#ifndef JOBCONTEXT_H
#define JOBCONTEXT_H

#include <chrono>

namespace jobsystem {
class JobContext {
protected:
  size_t m_frame_number;
  std::time_t m_timestamp;

public:
  JobContext(size_t frame_number, std::time_t timestamp)
      : m_frame_number{frame_number}, m_timestamp{timestamp} {}
};
} // namespace jobsystem

#endif /* JOBCONTEXT_H */
