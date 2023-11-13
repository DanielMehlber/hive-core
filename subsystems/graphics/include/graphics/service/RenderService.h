#ifndef RENDERSERVICE_H
#define RENDERSERVICE_H

#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "services/executor/impl/LocalServiceExecutor.h"

namespace graphics {

class RenderService : public services::impl::LocalServiceExecutor {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

public:
  explicit RenderService(common::subsystems::SharedSubsystemManager subsystems);

  std::future<services::SharedServiceResponse>
  Render(services::SharedServiceRequest request);
};

} // namespace graphics

#endif // RENDERSERVICE_H
