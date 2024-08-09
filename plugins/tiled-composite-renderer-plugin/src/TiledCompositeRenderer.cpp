#include "TiledCompositeRenderer.h"
#include "common/assert/Assert.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/SimpleViewMatrix.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include "services/registry/IServiceRegistry.h"

using namespace std::chrono_literals;
using namespace hive;

TiledCompositeRenderer::TiledCompositeRenderer(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    common::memory::Owner<graphics::IRenderer> &&output_renderer)
    : m_subsystems(subsystems), m_output_renderer(std::move(output_renderer)),
      m_current_service_count{0} {

  /* INITIALIZE SCENE CAMERA */
  auto [width, height] = m_output_renderer->GetCurrentSize();
  auto perspective =
      vsg::Perspective::create(60, (double)width / (double)height, 0.01, 1000);
  auto look_at = vsg::LookAt::create(vsg::dvec3{0, 1, 0}, vsg::dvec3{0, 0, 0},
                                     vsg::dvec3{0, 0, 1});
  m_camera = vsg::Camera::create(perspective, look_at);

  /* ADJUST CAMERA FOR CAPTURING TILED QUADS */
  m_output_renderer->SetCameraProjectionMatrix(
      vsg::Orthographic::create(0, 1, 0, 1, 0.1, 10));
  m_output_renderer->SetCameraViewMatrix(vsg::LookAt::create(
      vsg::dvec3{0, 0, 1}, vsg::dvec3{0, 0, -1}, vsg::dvec3{0, 1, 0}));

  m_frames_per_second = std::make_shared<std::atomic<int>>(0);
  auto job = std::make_shared<TimerJob>(
      [frames_per_second = std::weak_ptr<std::atomic<int>>(
           m_frames_per_second)](jobsystem::JobContext *context) {
        if (frames_per_second.expired()) {
          return JobContinuation::DISPOSE;
        }

        auto fps = frames_per_second.lock();
        int _fps = fps->exchange(0);

        LOG_DEBUG("Current Tiled Composite Renderer FPS: " << _fps)

        return JobContinuation::REQUEUE;
      },
      "render-debug-fps", 1s, CLEAN_UP);

  auto job_system =
      m_subsystems.Borrow()->RequireSubsystem<jobsystem::JobManager>();
  job_system->KickJob(job);

  auto camera_move_job = std::make_shared<TimerJob>(
      [weak_camera_ptr = vsg::observer_ptr<vsg::Camera>(m_camera)](
          jobsystem::JobContext *context) {
        if (!weak_camera_ptr) {
          return JobContinuation::DISPOSE;
        }
        auto time = std::chrono::high_resolution_clock::now();
        auto duration_in_seconds =
            std::chrono::duration<double>(time.time_since_epoch());
        auto x = duration_in_seconds.count();

        double y_pos = sin(x);
        double x_pos = cos(x);

        double distance = 10;
        auto camera = weak_camera_ptr.ref_ptr();
        auto look_at = camera->viewMatrix.cast<vsg::LookAt>();
        look_at->eye.y = distance * y_pos;
        look_at->eye.x = distance * x_pos;
        look_at->eye.z = 0;

        return JobContinuation::REQUEUE;
      },
      "eval-camera-mover", 1s / 20, MAIN);

  job_system->KickJob(camera_move_job);
}

std::optional<graphics::SharedRenderResult>
TiledCompositeRenderer::GetResult() {
  return m_output_renderer->GetResult();
}

void TiledCompositeRenderer::SetScene(const scene::SharedScene &scene){
    LOG_WARN("Cannot set scene using the TiledCompositeRenderer")}

scene::SharedScene TiledCompositeRenderer::GetScene() const {
  LOG_WARN("Cannot get scene using the TiledCompositeRenderer")
  return nullptr;
}

void TiledCompositeRenderer::Resize(int width, int height) {
  m_output_renderer->Resize(width, height);
}

std::tuple<int, int> TiledCompositeRenderer::GetCurrentSize() const {
  return m_output_renderer->GetCurrentSize();
}

void TiledCompositeRenderer::SetCameraProjectionMatrix(
    vsg::ref_ptr<vsg::ProjectionMatrix> matrix) {
  m_camera->projectionMatrix = matrix;
}

void TiledCompositeRenderer::SetCameraViewMatrix(
    vsg::ref_ptr<vsg::ViewMatrix> matrix) {
  m_camera->viewMatrix = matrix;
}

bool TiledCompositeRenderer::Render() {
  if (m_current_service_count < 1) {
    LOG_ERR("No render service available")
    return true;
  }

  auto subsystems = m_subsystems.Borrow();

  std::unique_lock lock(m_image_buffers_and_tiling_mutex);

  auto job_system = subsystems->RequireSubsystem<jobsystem::JobManager>();

  auto service_registry =
      subsystems->RequireSubsystem<services::IServiceRegistry>();

  auto future_caller = service_registry->Find("render");
  job_system->WaitForCompletion(future_caller);

  auto maybe_caller = future_caller.get();
  if (!maybe_caller.has_value()) {
    LOG_ERR("no render service available")
    return true;
  }

  auto service_caller = maybe_caller->get();

  auto counter = std::make_shared<jobsystem::job::JobCounter>();
  for (int i = 0; i < m_tile_infos.size(); i++) {

    Tile current_tile = m_tile_infos[i];

    DEBUG_ASSERT(current_tile.projection.valid(),
                 "tile information must contain valid projection")
    DEBUG_ASSERT(current_tile.relative_view_matrix.valid(),
                 "tile information must contain valid relative view matrix")

    auto perspective = current_tile.projection.cast<vsg::Perspective>();
    DEBUG_ASSERT(perspective.valid(), "tile must have valid perspective")

    auto view_matrix = current_tile.relative_view_matrix->transform() *
                       m_camera->viewMatrix->transform();

    // build render service request
    graphics::RenderServiceRequest request;
    request.SetExtend({current_tile.width, current_tile.height});
    request.SetViewMatrix(view_matrix);
    request.SetPerspectiveProjection(graphics::Perspective{
        perspective->nearDistance, perspective->farDistance,
        perspective->fieldOfViewY});

    auto rendering_service_request = request.GetRequest();

    auto job = std::make_shared<jobsystem::job::Job>(
        [_this = ReferenceFromThis(), rendering_service_request,
         subsystems = m_subsystems, service_caller,
         i](jobsystem::JobContext *context) mutable {
          auto future_response = service_caller->IssueCallAsJob(
              rendering_service_request, context->GetJobManager());

          context->GetJobManager()->WaitForCompletion(future_response);
          auto response = future_response.get();

          if (response->GetStatus() == services::OK) {
            auto color_buffer_encoded =
                std::move(response->GetResult("color").value());

            auto encoding = response->GetResult("encoding").value();
            auto decoder =
                subsystems.Borrow()
                    ->RequireSubsystem<graphics::IRenderResultEncoder>();

            if (encoding == decoder->GetName()) {
              auto color_buffer =
                  std::move(decoder->Decode(color_buffer_encoded));

              // TODO: read format from image instead assuming it
              auto image_buffer = _this.Borrow()->m_image_buffers[i];
              image_buffer->properties.format = VK_FORMAT_R8G8B8A8_UNORM;
              std::memcpy(image_buffer->dataPointer(), color_buffer.data(),
                          color_buffer.size());
              image_buffer->dirty();

            } else {
              LOG_ERR("Cannot decode image from remote render result because "
                      "decoder for "
                      << encoding << " is not registered")
              return JobContinuation::DISPOSE;
            }
          }

          return JobContinuation::DISPOSE;
        },
        "remote-render-tile-" + std::to_string(i));
    job->AddCounter(counter);
    job_system->KickJob(job);
  }

  job_system->WaitForCompletion(counter);
  m_output_renderer->Render();

#ifndef NDEBUG
  m_frames_per_second->operator++();
#endif

  return true;
}

void TiledCompositeRenderer::SetServiceCount(size_t services) {
  std::unique_lock lock(m_image_buffers_and_tiling_mutex);
  UpdateTilingInfo(services);
  CreateSceneWithTilingQuads();
}

void subdividePerspectiveFrustum(
    int width, int height,
    const vsg::ref_ptr<vsg::ProjectionMatrix> &projection, Tile &tile) {

  DEBUG_ASSERT(projection.valid(), "projection matrix should not be null")

  float width_percentage = (float)tile.width / (float)width;

  /* CALCULATE NEW PROJECTION MATRIX */
  auto perspective = projection.cast<vsg::Perspective>();
  tile.projection = perspective;

  /* CALCULATE NEW VIEW MATRIX */

  double tile_center_x = tile.x + (double)tile.width / 2;
  // vertical distance between image center and tile center for rotation
  double delta_x = tile_center_x - (double)width / 2;
  // degree to rotate view to point to towards tile center
  double degree_x = (delta_x / (double)width) * perspective->fieldOfViewY;

  double radians_x = (degree_x * M_PI) / 180.0;
  vsg::dmat4 relative_rotation_matrix = vsg::rotate(radians_x, 0.0, 1.0, 0.0);

  // TODO: Support horizontal tiling

  tile.relative_view_matrix =
      graphics::SimpleViewMatrix::create(relative_rotation_matrix);
}

void TiledCompositeRenderer::UpdateTilingInfo(size_t tile_count) {
  m_current_service_count = tile_count;

  auto extend = GetCurrentSize();
  auto width = std::get<0>(extend);
  auto height = std::get<1>(extend);

  m_image_buffers.clear();
  m_image_buffers.resize(tile_count);

  m_tile_infos.clear();
  m_tile_infos.resize(tile_count);

  if (tile_count > 0) {

    int tile_width = width / static_cast<int>(m_current_service_count);
    int tile_height = height;

    for (int i = 0; i < m_current_service_count; i++) {
      vsg::ref_ptr<vsg::Data> image_buffer;
      image_buffer =
          vsg::Array2D<vsg::t_vec4<char>>::create(tile_width, tile_height);
      image_buffer->properties.dataVariance = vsg::DYNAMIC_DATA;
      image_buffer->properties.format = VK_FORMAT_R8G8B8A8_UNORM;
      m_image_buffers[i] = image_buffer;

      Tile tile{i * tile_width, 0, tile_width, tile_height, i};

      subdividePerspectiveFrustum(width, height, m_camera->projectionMatrix,
                                  tile);

      m_tile_infos[i] = tile;
    }
  }
}

void TiledCompositeRenderer::CreateSceneWithTilingQuads() {
  auto scene = std::make_shared<scene::SceneManager>();
  auto root = scene->GetRoot();

  float width, height;
  std::tie(width, height) = GetCurrentSize();

  float i = 0;
  for (const auto &tile_info : m_tile_infos) {
    /*
     * Use a fresh builder for every quad. It maintains some weird internal
     * state that causes unwanted behavior otherwise.
     */
    vsg::Builder builder;
    vsg::GeometryInfo geomInfo;
    i++;
    float x = static_cast<float>(tile_info.x) / width;
    float y = static_cast<float>(tile_info.y) / height;
    float dx = static_cast<float>(tile_info.width) / width;
    float dy = static_cast<float>(tile_info.height) / height;

    geomInfo.position = {x + dx / 2, y + dy / 2, 0};
    geomInfo.dx = {dx, 0, 0};
    geomInfo.dy = {0, dy, 0};
    geomInfo.dz = {0, 0, 0};

    vsg::StateInfo stateInfo;
    stateInfo.two_sided = true;
    stateInfo.image = m_image_buffers[tile_info.image_index];

    auto quad = builder.createQuad(geomInfo, stateInfo);

    root->addChild(quad);
  }

  m_output_renderer->SetScene(scene);
}