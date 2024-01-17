#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"

#ifndef TILEDCOMPOSITERENDERER_H
#define TILEDCOMPOSITERENDERER_H

struct Tile {
  int x;
  int y;
  int width;
  int height;
  int image_index;
  vsg::ref_ptr<vsg::ProjectionMatrix> projection;
  vsg::ref_ptr<vsg::ViewMatrix> relative_view_matrix;
};

/**
 * @brief A renderer that divides the overall render surface into tiles and uses
 * provided rendering services to render the necessary tiles.
 */
class TiledCompositeRenderer
    : public graphics::IRenderer,
      public std::enable_shared_from_this<TiledCompositeRenderer> {
private:
  graphics::SharedRenderer m_output_renderer;
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  vsg::ref_ptr<vsg::Camera> m_camera;

  size_t m_current_service_count;
  std::vector<vsg::ref_ptr<vsg::Data>> m_image_buffers;
  std::vector<Tile> m_tile_infos;
  mutable std::mutex m_image_buffers_and_tiling_mutex;

#ifndef NDEBUG
  std::shared_ptr<std::atomic<int>> m_frames_per_second;
#endif

  void UpdateTilingInfo(size_t tile_count);
  void CreateSceneWithTilingQuads();

public:
  TiledCompositeRenderer(
      const common::subsystems::SharedSubsystemManager &subsystems,
      graphics::SharedRenderer output_renderer);

  /**
   * Render current scene from current view
   */
  bool Render() override;

  void SetServiceCount(size_t services);

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

  graphics::RendererSetup GetSetup() const override;
};

inline graphics::RendererSetup TiledCompositeRenderer::GetSetup() const {
  return m_output_renderer->GetSetup();
}

#endif // TILEDCOMPOSITERENDERER_H
