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
  vsg::ref_ptr<vsg::ProjectionMatrix> m_current_projection_matrix;
  vsg::ref_ptr<vsg::ViewMatrix> m_current_view_matrix;

  vsg::ref_ptr<vsg::Commands> m_depth_buffer_capture;
  vsg::ref_ptr<vsg::Commands> m_color_buffer_capture;

  vsg::ref_ptr<vsg::ImageView> m_color_image_view;
  vsg::ref_ptr<vsg::ImageView> m_depth_image_view;

  vsg::ref_ptr<vsg::Image> m_copied_color_buffer;
  vsg::ref_ptr<vsg::Buffer> m_copied_depth_buffer;

  vsg::ref_ptr<vsg::StateGroup> m_scene_graph_root;

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
  void SetupInstanceAndDevice(const RendererInfo &pre_init_info = {});
  void SetupCommandGraph();

  /**
   * Replaces framebuffer and image captures
   * @note This can be used when the render size has changed.
   * @see Inspired by vsgExample vsgheadless
   */
  void ReplaceFramebufferAndCaptures();

public:
  /**
   * Constructs an offscreen renderer that writes images to buffers instead of
   * displaying them on a screen.
   * @param scene optional scene to directly set in the renderer
   * @param use_depth_buffer if true, captures depth buffer as well. This can
   * increase performance cost.
   * @param msaa if true, uses multi-sampled anti aliasing
   * @param m_imageFormat specifies vulkan image format of color image copied to
   * buffer
   * @param m_depthFormat specifies vulkan image format of depth image copied to
   * buffer
   * @param m_sample_count sample count for multisampling
   * @param m_size initial size of image to render (can be changed later)
   * @param pre_init_info if the device and instance are already set-up, they
   * can be passed using the renderer info. This renderer will then use the
   * provided ones and does not create its own.
   */
  explicit OffscreenRenderer(
      RendererInfo pre_init_info = {},
      scene::SharedScene scene = std::make_shared<scene::SceneManager>(),
      bool use_depth_buffer = true, bool msaa = false,
      VkFormat m_imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
      VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT,
      VkSampleCountFlagBits m_sample_count = VK_SAMPLE_COUNT_1_BIT,
      VkExtent2D m_size = {2048, 1024});

  bool Render() override;

  /**
   * Reads render results from captured images of framebuffer
   * @see Inspired by vsgExample vsgheadless
   * @return
   */
  std::optional<SharedRenderResult> GetResult() override;

  void SetScene(const scene::SharedScene &scene) override;
  scene::SharedScene GetScene() const override;

  void Resize(int width, int height) override;
  std::tuple<int, int> GetCurrentSize() const override;

  void SetCameraProjectionMatrix(
      vsg::ref_ptr<vsg::ProjectionMatrix> matrix) override;
  void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) override;

  RendererInfo GetInfo() const override;
};

inline RendererInfo OffscreenRenderer::GetInfo() const {
  return {m_device, m_instance};
}

} // namespace graphics

#endif // OFFSCREENRENDERER_H
