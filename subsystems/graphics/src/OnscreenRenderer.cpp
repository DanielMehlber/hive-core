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

OnscreenRenderer::OnscreenRenderer(scene::SharedScene scene, int width,
                                   int height)
    : m_scene(std::move(scene)) {
  auto windowTraits = vsg::WindowTraits::create();
  windowTraits->width = width;
  windowTraits->height = height;

#ifndef NDEBUG
  // windowTraits->debugLayer = true;
#endif

  m_window = vsg::Window::create(windowTraits);
  if (!m_window) {
    LOG_ERR("Cannot create window for Onscreen Renderer");
    return;
  }

  m_viewer = vsg::Viewer::create();
  m_viewer->addWindow(m_window);

  // add close handler to respond to the close window button and pressing escape
  m_viewer->addEventHandler(vsg::CloseHandler::create(m_viewer));

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
