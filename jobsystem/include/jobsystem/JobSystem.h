#ifndef JOBSYSTEM_H
#define JOBSYSTEM_H

#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/execution/IJobExecution.h"
#include "jobsystem/execution/JobExecutionState.h"
#include "jobsystem/execution/impl/fiber/FiberExecutionImpl.h"
#include "jobsystem/execution/impl/singleThreaded/SingleThreadedExecutionImpl.h"
#include "jobsystem/manager/JobManager.h"

// utility
#define JOB(x) jobsystem::JobSystemFactory::CreateJob(x)
#define JOB_COUNTER() jobsystem::JobSystemFactory::CreateCounter()

#endif /* JOBSYSTEM_H */
