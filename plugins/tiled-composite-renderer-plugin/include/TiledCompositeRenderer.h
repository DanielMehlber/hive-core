#ifndef TILEDCOMPOSITERENDERER_H
#define TILEDCOMPOSITERENDERER_H

#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/synchronization/JobMutex.h"

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
 * A renderer that divides the overall render surface into tiles and uses
 * provided rendering services to render the necessary tiles.
 */
class TiledCompositeRenderer
    : public graphics::IRenderer,
      public common::memory::EnableBorrowFromThis<TiledCompositeRenderer> {
private:
  common::memory::Owner<graphics::IRenderer> m_output_renderer;
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  vsg::ref_ptr<vsg::Camera> m_camera;

  size_t m_current_service_count;
  std::vector<vsg::ref_ptr<vsg::Data>> m_image_buffers;
  std::vector<Tile> m_tile_infos;
  mutable jobsystem::mutex m_image_buffers_and_tiling_mutex;

  std::shared_ptr<std::atomic<int>> m_frames_per_second;

  void UpdateTilingInfo(size_t tile_count);
  void CreateSceneWithTilingQuads();

public:
  TiledCompositeRenderer(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      common::memory::Owner<graphics::IRenderer> &&output_renderer);

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
