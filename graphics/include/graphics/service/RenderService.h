#ifndef RENDERSERVICE_H
#define RENDERSERVICE_H

#include "graphics/renderer/IRenderer.h"
#include "services/executor/impl/LocalServiceExecutor.h"

namespace graphics {

class RenderService : public services::impl::LocalServiceExecutor {
private:
  SharedRenderer m_renderer;

public:
  explicit RenderService(SharedRenderer renderer);

  std::future<services::SharedServiceResponse>
  Render(services::SharedServiceRequest request);
};

} // namespace graphics

#endif // RENDERSERVICE_H
