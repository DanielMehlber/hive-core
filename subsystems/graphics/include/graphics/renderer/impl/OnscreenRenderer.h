#ifndef ONSCREENRENDERER_H
#define ONSCREENRENDERER_H

#include "graphics/renderer/IRenderer.h"

namespace graphics {

/**
 * Renderer creating a window and displaying its image results on it.
 */
class OnscreenRenderer : public IRenderer {
protected:
  scene::SharedScene m_scene;
  vsg::ref_ptr<vsg::Window> m_window;
  vsg::ref_ptr<vsg::Viewer> m_viewer;
  vsg::ref_ptr<vsg::Camera> m_camera;

  void SetupCamera();
  void SetupCommandGraph();

public:
  explicit OnscreenRenderer(
      scene::SharedScene scene = std::make_shared<scene::SceneManager>(),
      int width = 500, int height = 500);

  bool Render() override;

  std::optional<SharedRenderResult> GetResult() override;

  void SetScene(const scene::SharedScene &scene) override;
  scene::SharedScene GetScene() const override;

  void Resize(int width, int height) override;
  std::tuple<int, int> GetCurrentSize() const override;

  void SetCameraProjectionMatrix(
      vsg::ref_ptr<vsg::ProjectionMatrix> matrix) override;
  void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) override;

  RendererSetup GetSetup() const override;
};

inline RendererSetup OnscreenRenderer::GetSetup() const {
  return {m_window->getDevice(), m_window->getInstance()};
}

} // namespace graphics

#endif // ONSCREENRENDERER_H
