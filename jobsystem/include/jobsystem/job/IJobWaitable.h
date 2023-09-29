#ifndef IJOBWAITABLE_H
#define IJOBWAITABLE_H

namespace jobsystem {

/**
 * @brief Generic interface for objects and processes that can be waited for
 * during job execution.
 */
class IJobWaitable {
public:
  /**
   * @brief Checks if the object has finished and the control flow may continue.
   * @return true, if there is no need to wait any longer and the control flow
   * can be continued.
   */
  virtual bool IsFinished() = 0;
};
} // namespace jobsystem

#endif /* IJOBWAITABLE_H */
