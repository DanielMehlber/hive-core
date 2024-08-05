#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <future>

namespace hive::graphics {

DECLARE_EXCEPTION(WindowCreationFailedException);

/**
 * Renderer creating a window and displaying its image results on it.
 */
class OnscreenRenderer : public IRenderer {
protected:
  scene::SharedScene m_scene;
  vsg::ref_ptr<vsg::Window> m_window;
  vsg::ref_ptr<vsg::Viewer> m_viewer;
  vsg::ref_ptr<vsg::Camera> m_camera;

#ifdef WIN32
  /**
   * In contrast to linux, Windows OS requires that the same thread that creates
   * the window (called WinProc) is also the one handling messages from the
   * Windows API. Otherwise, the spawned window is marked as non-responsive.
   * This is the WinProc thread (only required on Windows OS).
   */
  std::thread m_win_proc_thread;

  /**
   * The viewer is now accessed simultaneously by the WinProc and rendering job.
   * This mutex synchronizes their access and avoid double message/event
   * handling.
   */
  jobsystem::mutex m_viewer_mutex;
#endif

  void SetupCamera();
  void SetupCommandGraph();
  void SetupWindow(int width, int height);

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

} // namespace hive::graphics
