#include "VulkanOperations.h"
#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"

#ifndef TILEDCOMPOSITERENDERER_H
#define TILEDCOMPOSITERENDERER_H

/**
 * @brief A renderer that divides the overall render surface into tiles and uses
 * provided rendering services to render the necessary tiles.
 */

struct CameraInfo {
  vsg::dvec3 position;
  vsg::dvec3 direction;
  vsg::dvec3 up;
};

class TiledCompositeRenderer
    : public graphics::IRenderer,
      public std::enable_shared_from_this<TiledCompositeRenderer> {
private:
  graphics::SharedRenderer m_output_renderer;
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;
  CameraInfo m_camera_info;
  vsg::ref_ptr<vsg::Camera> m_camera;

#ifndef NDEBUG
  std::shared_ptr<std::atomic<int>> m_frames_per_second;
#endif

  vsg::ref_ptr<vsg::Image>
  LoadBufferIntoImage(const std::vector<unsigned char> &buffer, VkFormat format,
                      uint32_t width, uint32_t height) const;

public:
  TiledCompositeRenderer(
      const common::subsystems::SharedSubsystemManager &subsystems,
      graphics::SharedRenderer output_renderer);

  /**
   * Render current scene from current view
   */
  bool Render() override;

  /**
   * @return Last RenderResult (if one exists)
   */
  std::optional<graphics::SharedRenderResult> GetResult() override;

  void SetScene(const scene::SharedScene &scene) override;
  scene::SharedScene GetScene() const override;

  void Resize(int width, int height) override;
  std::tuple<int, int> GetCurrentSize() const override;

  void SetCameraProjectionMatrix(
      vsg::ref_ptr<vsg::ProjectionMatrix> matrix) override;
  void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) override;

  vsg::ref_ptr<vsg::Device> GetVulkanDevice() const override;
};

#endif // TILEDCOMPOSITERENDERER_H
