#ifndef RENDERSERVICE_H
#define RENDERSERVICE_H

#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "services/executor/impl/LocalServiceExecutor.h"

namespace graphics {

class RenderService : public services::impl::LocalServiceExecutor {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;
  std::shared_ptr<graphics::IRenderer> m_renderer;
  mutable std::mutex m_service_mutex;

public:
  explicit RenderService(
      const common::subsystems::SharedSubsystemManager &subsystems,
      std::shared_ptr<graphics::IRenderer> renderer = nullptr);

  std::future<services::SharedServiceResponse>
  Render(const services::SharedServiceRequest &request);

  std::optional<graphics::SharedRenderer> GetRenderer() const;
};

} // namespace graphics

#endif // RENDERSERVICE_H
