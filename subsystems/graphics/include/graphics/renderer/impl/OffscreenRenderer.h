#pragma once

#include "graphics/renderer/IRenderer.h"
#include "vsg/all.h"

namespace graphics {

/**
 * Renderer that does not display its images on a window surface but instead
 * transfers them into a buffer for further processing.
 * @note Built for the rendering service, embedded rendering, or other
 * applications not involving a window.
 */
class OffscreenRenderer : public IRenderer {
private:
  /** If true, the depth buffer is extracted and stored as well. */
  bool m_use_depth_buffer;

  /** If true, uses multi-sampled anti-aliasing. */
  bool m_msaa;

  /** Current scene that is rendered */
  scene::SharedScene m_scene;

  vsg::ref_ptr<vsg::Viewer> m_viewer;
  vsg::ref_ptr<vsg::Instance> m_instance;
  vsg::ref_ptr<vsg::Device> m_device;

  vsg::ref_ptr<vsg::Camera> m_camera;
  vsg::ref_ptr<vsg::ProjectionMatrix> m_current_projection_matrix;
  vsg::ref_ptr<vsg::ViewMatrix> m_current_view_matrix;

  /**
   * Contains commands for capturing and extracting the depth image in the
   * current frame buffer into a separate image buffer on device memory.
   */
  vsg::ref_ptr<vsg::Commands> m_depth_buffer_capture;

  /**
   * Contains commands for capturing and extracting the color image in the
   * current frame buffer into a separate image buffer on device memory.
   */
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

  /** Render result of last rendering pass. */
  std::shared_ptr<RenderResult> m_current_render_result;

  void SetupCamera();
  void SetupRenderGraph();
  void SetupInstanceAndDevice(
      const std::optional<RendererSetup> &existing_renderer_setup = {});
  void SetupCommandGraph();

  /**
   * Replaces framebuffer and image captures.
   * @note This can be used when the renderer size has changed.
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
   * @param existing_renderer_setup if the device and instance are already
   * set-up, they can be passed using the renderer info. This renderer will then
   * use the provided ones and does not create its own.
   */
  explicit OffscreenRenderer(
      const std::optional<RendererSetup> &existing_renderer_setup = {},
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

  RendererSetup GetSetup() const override;
};

inline RendererSetup OffscreenRenderer::GetSetup() const {
  return {m_device, m_instance};
}

} // namespace graphics
