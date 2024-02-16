#include <utility>

#include "common/profiling/Timer.h"
#include "graphics/renderer/impl/OnscreenRenderer.h"
#include "logging/LogManager.h"

using namespace graphics;

void OnscreenRenderer::SetupCamera() {

  auto nearFarRatio = 0.001;
  auto horizonMountainHeight = 0.0;

  // set up the camera
  auto centre = vsg::dvec3{0.0, 0.0, 0.0};
  auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -10, 0.0), centre,
                                    vsg::dvec3(0.0, 0.0, 1.0));

  vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
  auto ellipsoidModel =
      m_scene->GetRoot()->getRefObject<vsg::EllipsoidModel>("EllipsoidModel");
  if (ellipsoidModel) {
    perspective = vsg::EllipsoidPerspective::create(
        lookAt, ellipsoidModel, 30.0,
        static_cast<double>(m_window->extent2D().width) /
            static_cast<double>(m_window->extent2D().height),
        nearFarRatio, horizonMountainHeight);
  } else {
    perspective = vsg::Perspective::create(
        30.0,
        static_cast<double>(m_window->extent2D().width) /
            static_cast<double>(m_window->extent2D().height),
        0.001, 2000);
  }

  m_camera = vsg::Camera::create(
      perspective, lookAt, vsg::ViewportState::create(m_window->extent2D()));

  m_viewer->addEventHandler(vsg::Trackball::create(m_camera, ellipsoidModel));
}

void OnscreenRenderer::SetupCommandGraph() {
  auto commandGraph =
      vsg::createCommandGraphForView(m_window, m_camera, m_scene->GetRoot());
  m_viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
  m_viewer->compile();
}

void OnscreenRenderer::SetupWindow(int width, int height) {
  auto windowTraits = vsg::WindowTraits::create();
  windowTraits->width = width;
  windowTraits->height = height;

#ifndef NDEBUG
  // windowTraits->debugLayer = true;
#endif

  m_window = vsg::Window::create(windowTraits);

  if (!m_window) {
    LOG_ERR("failed to create window for unknown reason")
    return;
  }

  m_viewer = vsg::Viewer::create();
  m_viewer->addWindow(m_window);

  // add close handler to respond to the close window button and pressing escape
  m_viewer->addEventHandler(vsg::CloseHandler::create(m_viewer));
}

OnscreenRenderer::OnscreenRenderer(scene::SharedScene scene, int width,
                                   int height)
    : m_scene(std::move(scene)) {

#ifdef WIN32
  /*
   * *** Windows workaround ahead ***
   *
   * Introduction: Operating systems send messages/events to running windows
   * (mouse events, pressed keys, and various others). The window must handle
   * (or respond) to them. If it does not for some time period, the window is
   * marked as non-responsive. The Windows OS, however, additionally requires
   * these events/messages to be processed by the same thread, that initially
   * created the window. This dedicated thread responsible for managing the
   * window during its entire lifetime is called the WinProc (Window Procedure).
   *
   * Problem: Because the job-system possibly schedules the rendering task
   * (which includes processing of window events) onto different worker threads
   * each time, events are usually not handled by the same WinProc thread that
   * created the window. As consequence, events of the Windows API are not
   * handled properly and the window is marked as non-responsive.
   *
   * Solution: Create a dedicated WinProc thread that creates the window AND
   * handles all of its events.
   *
   * Personal Rant: Shouldn't it be irrelevant to the OS which thread polls and
   * handles a window's events as long as they are handled at all? Somehow, this
   * is not an issue on Linux. I'd love to hear the reason for this API design
   * decision of Microsoft.
   */
  std::promise<void> window_promise;
  std::future<void> window_future = window_promise.get_future();

  // Set up the WinProc thread for window creation and event handling.
  m_win_proc_thread = std::thread(
      [this, promise = std::move(window_promise), width, height]() mutable {
        // 1) create window and notify main thread that it can continue
        this->SetupWindow(width, height);
        promise.set_value();

        // 2) do not process events if the window creation failed
        if (!m_window) {
          return;
        }

        // 3) poll and handle events in a synchronized manner
        while (m_viewer) {
          std::unique_lock lock(m_viewer_mutex);
          m_viewer->pollEvents(true);
          m_viewer->handleEvents();
        }
      });

  // only continue as soon as window has been setup
  window_future.wait();
#else
  // linux does not require a dedicated WinProc thread.
  SetupWindow(width, height);
#endif

  if (!m_window) {
    LOG_ERR("Cannot create window for Onscreen Renderer");
    return;
  }

  SetupCamera();
  SetupCommandGraph();

  m_viewer->start_point() = vsg::clock::now();
}

bool OnscreenRenderer::Render() {
#ifdef ENABLE_PROFILING
  auto [width, height] = GetCurrentSize();
  common::profiling::Timer render_timer("onscreen-rendering-" +
                                        std::to_string(width) + "x" +
                                        std::to_string(height));
#endif

#ifdef WIN32
  // required on windows to avoid data races between the WinProc thread and this
  // thread/fiber both accessing the viewer.
  std::unique_lock lock(m_viewer_mutex);
#endif

  if (m_viewer->advanceToNextFrame()) {
    m_viewer->handleEvents();
    m_viewer->update();
    m_viewer->recordAndSubmit();
    m_viewer->present();
    return true;
  } else {
    return false;
  }
}

std::optional<SharedRenderResult> OnscreenRenderer::GetResult() {
  LOG_WARN("RenderResults are not supported for the OnscreenRenderer");
  return {};
}

void OnscreenRenderer::SetScene(const scene::SharedScene &scene) {
  this->m_scene = scene;
  SetupCommandGraph();
}

void OnscreenRenderer::Resize(int width, int height) {}

scene::SharedScene OnscreenRenderer::GetScene() const { return m_scene; }

std::tuple<int, int> OnscreenRenderer::GetCurrentSize() const {
  return std::make_tuple(m_window->extent2D().width,
                         m_window->extent2D().height);
}

void OnscreenRenderer::SetCameraProjectionMatrix(
    vsg::ref_ptr<vsg::ProjectionMatrix> matrix) {
  m_camera->projectionMatrix = matrix;
}

void OnscreenRenderer::SetCameraViewMatrix(
    vsg::ref_ptr<vsg::ViewMatrix> matrix) {
  m_camera->viewMatrix = matrix;
}
