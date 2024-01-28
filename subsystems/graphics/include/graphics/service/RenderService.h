#ifndef RENDERSERVICE_H
#define RENDERSERVICE_H

#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "services/executor/impl/LocalServiceExecutor.h"

namespace graphics {

/**
 * Service that offers rendering an image of the current scene and returning its
 * bytes as buffer to the caller.
 * @note For now, parallel rendering is not supported. Render requests are
 * executed sequentially.
 */
class RenderService : public services::impl::LocalServiceExecutor {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  /** Renderer that should be used for executing the rendering request */
  std::shared_ptr<graphics::IRenderer> m_renderer;

  /**
   * Fow now, only one image can be taken at a time. This mutex enforces this
   * rule.
   */
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
