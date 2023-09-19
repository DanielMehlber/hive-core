#ifndef IJOBWAITABLE_H
#define IJOBWAITABLE_H

namespace jobsystem {
class IJobWaitable {
public:
  virtual bool IsFinished() = 0;
};
} // namespace jobsystem

#endif /* IJOBWAITABLE_H */
