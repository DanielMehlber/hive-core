#ifndef OFFSCREENRENDERER_H
#define OFFSCREENRENDERER_H

#include "graphics/Graphics.h"
#include "graphics/renderer/IRenderer.h"
#include "vsg/all.h"

namespace graphics {

class GRAPHICS_API OffscreenRenderer : public IRenderer {
private:
  bool m_use_depth_buffer;
  bool m_msaa;

  scene::SharedScene m_scene;
  vsg::ref_ptr<vsg::Viewer> m_viewer;
  vsg::ref_ptr<vsg::Instance> m_instance;
  vsg::ref_ptr<vsg::Device> m_device;
  vsg::ref_ptr<vsg::Camera> m_camera;

  vsg::ref_ptr<vsg::Commands> m_depth_buffer_capture;
  vsg::ref_ptr<vsg::Commands> m_color_buffer_capture;

  vsg::ref_ptr<vsg::ImageView> m_color_image_view;
  vsg::ref_ptr<vsg::ImageView> m_depth_image_view;

  vsg::ref_ptr<vsg::Framebuffer> m_framebuffer;
  vsg::ref_ptr<vsg::Image> m_copied_color_buffer;
  vsg::ref_ptr<vsg::Buffer> m_copied_depth_buffer;

  int m_queueFamily;

  VkExtent2D m_size{2048, 1024};
  VkFormat m_color_image_format = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat m_depth_image_format = VK_FORMAT_D32_SFLOAT;

  VkSampleCountFlagBits m_sample_count = VK_SAMPLE_COUNT_1_BIT;

  vsg::ref_ptr<vsg::CommandGraph> m_command_graph;
  vsg::ref_ptr<vsg::RenderGraph> m_render_graph;

  std::shared_ptr<RenderResult> m_current_render_result;

  void SetupCamera();
  void SetupRenderGraph();
  void SetupInstanceAndDevice();
  void SetupCommandGraph();

public:
  OffscreenRenderer(
      scene::SharedScene scene = std::make_shared<scene::SceneManager>(),
      bool use_depth_buffer = true, bool msaa = false,
      VkFormat m_imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
      VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT,
      VkSampleCountFlagBits m_sample_count = VK_SAMPLE_COUNT_1_BIT,
      VkExtent2D m_size = {2048, 1024});

  bool Render() override;

  std::optional<SharedRenderResult> GetResult() override;

  void SetScene(const scene::SharedScene &scene) override;
  scene::SharedScene GetScene() const override;

  void Resize(int width, int height) override;
  std::tuple<int, int> GetCurrentSize() const override;

  void SetCameraProjectionMatrix(
      vsg::ref_ptr<vsg::ProjectionMatrix> matrix) override;
  void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) override;
};

} // namespace graphics

#endif // OFFSCREENRENDERER_H
