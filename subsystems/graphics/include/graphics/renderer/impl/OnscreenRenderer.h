#ifndef ONSCREENRENDERER_H
#define ONSCREENRENDERER_H

#include "graphics/Graphics.h"
#include "graphics/renderer/IRenderer.h"

namespace graphics {
class GRAPHICS_API OnscreenRenderer : public IRenderer {
protected:
  scene::SharedScene m_scene;
  vsg::ref_ptr<vsg::Window> m_window;
  vsg::ref_ptr<vsg::Viewer> m_viewer;
  vsg::ref_ptr<vsg::Camera> m_camera;
  vsg::ref_ptr<vsg::Device> m_device;

  void SetupCamera();

public:
  explicit OnscreenRenderer(
      scene::SharedScene scene = std::make_shared<scene::SceneManager>());

  bool Render() override;

  std::optional<SharedRenderResult> GetResult() override;

  void SetScene(const scene::SharedScene &scene) override;
  scene::SharedScene GetScene() const override;

  void Resize(int width, int height) override;
  std::tuple<int, int> GetCurrentSize() const override;

  void SetCameraProjectionMatrix(
      vsg::ref_ptr<vsg::ProjectionMatrix> matrix) override;
  void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) override;

  vsg::ref_ptr<vsg::Device> GetVulkanDevice() const override;
};

inline vsg::ref_ptr<vsg::Device> OnscreenRenderer::GetVulkanDevice() const {
  return m_device;
}

} // namespace graphics

#endif // ONSCREENRENDERER_H
