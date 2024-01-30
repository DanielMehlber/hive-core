#ifndef JOBEXECUTIONSTATE_H
#define JOBEXECUTIONSTATE_H

namespace jobsystem::execution {

/**
 * current state and life-cycle of the job execution system.
 */
enum JobExecutionState { STOPPED, RUNNING };
} // namespace jobsystem::execution

#endif /* JOBEXECUTIONSTATE_H */
