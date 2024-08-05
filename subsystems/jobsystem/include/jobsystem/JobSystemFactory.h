#pragma once

#include "jobsystem/job/Job.h"
#include "jobsystem/synchronization/JobCounter.h"

using namespace hive::jobsystem::job;

namespace hive::jobsystem {

class JobSystemFactory {
public:
  template <typename Type = Job, typename... Args>
  static SharedJob CreateJob(Args... arguments);
  static SharedJobCounter CreateCounter();
};

template <typename Type, typename... Args>
inline SharedJob JobSystemFactory::CreateJob(Args... arguments) {
  return std::make_shared<Type>(arguments...);
}

inline SharedJobCounter JobSystemFactory::CreateCounter() {
  return std::make_shared<JobCounter>();
}
} // namespace hive::jobsystem
