#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "services/executor/impl/LocalServiceExecutor.h"

namespace hive::graphics {

/**
 * Service that offers rendering an image of the current scene and returning its
 * bytes as buffer to the caller.
 * @note For now, parallel rendering is not supported. Render requests are
 * executed sequentially.
 */
class RenderService : public services::impl::LocalServiceExecutor {
private:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  /** Renderer that should be used for executing the rendering request */
  common::memory::Reference<IRenderer> m_renderer;

  /**
   * Fow now, only one image can be taken at a time. This mutex enforces this
   * rule.
   */
  mutable jobsystem::mutex m_service_mutex;

public:
  explicit RenderService(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      common::memory::Reference<graphics::IRenderer> renderer = {});

  std::future<services::SharedServiceResponse>
  Render(const services::SharedServiceRequest &request);

  std::optional<common::memory::Borrower<graphics::IRenderer>> GetRenderer();
};

} // namespace hive::graphics
