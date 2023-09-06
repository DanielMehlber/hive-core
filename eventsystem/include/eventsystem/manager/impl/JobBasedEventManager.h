#ifndef JOBBASEDEVENTMANAGER_H
#define JOBBASEDEVENTMANAGER_H

#include "eventsystem/listeners/IEventListener.h"
#include "eventsystem/manager/IEventManager.h"
#include "jobsystem/JobManager.h"
#include <list>
#include <map>
#include <memory>
#include <vector>

using namespace jobsystem;

namespace eventsystem::impl {
class JobBasedEventManager final : public IEventManager {
private:
  std::map<std::string, std::vector<std::weak_ptr<IEventListener>>> m_listeners;
  std::shared_ptr<JobManager> m_job_manager;

  void CleanUpListeners();

public:
  JobBasedEventManager(std::shared_ptr<JobManager> job_manager);
  virtual ~JobBasedEventManager();

  virtual void FireEvent(EventRef event) override;

  virtual bool HasListener(const std::string &listener_id,
                           const std::string &type) const override;

  virtual void AddListener(std::weak_ptr<IEventListener> listener,
                           const std::string &type) override;

  virtual void RemoveListener(std::weak_ptr<IEventListener> listener) override;

  virtual void RemoveListenerFromType(std::weak_ptr<IEventListener> listener,
                                      const std::string &type) override;

  virtual void RemoveAllListeners() override;
};
} // namespace eventsystem::impl

#endif /* JOBBASEDEVENTMANAGER_H */
