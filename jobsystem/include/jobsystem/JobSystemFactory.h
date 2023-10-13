#ifndef JOBFACTORY_H
#define JOBFACTORY_H

#include "jobsystem/JobSystem.h"
#include "jobsystem/job/Job.h"
#include "jobsystem/job/JobCounter.h"

using namespace jobsystem::job;

namespace jobsystem {

class JOBSYSTEM_API JobSystemFactory {
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
} // namespace jobsystem

#endif /* JOBFACTORY_H */
