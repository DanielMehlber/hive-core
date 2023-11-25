#include "TiledCompositeRenderer.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "jobsystem/manager/JobManager.h"
#include "services/registry/IServiceRegistry.h"

using namespace std::chrono_literals;

TiledCompositeRenderer::TiledCompositeRenderer(
    const common::subsystems::SharedSubsystemManager &subsystems,
    graphics::SharedRenderer output_renderer)
    : m_subsystems(subsystems), m_output_renderer(std::move(output_renderer)) {

  auto projection_matrix =
      vsg::Orthographic::create(-0.5, 0.5, -0.5, 0.5, 0.1, 10);
  auto view_matrix = vsg::LookAt::create(
      vsg::dvec3{0, 0, 1}, vsg::dvec3{0, 0, -1}, vsg::dvec3{0, 1, 0});
  m_camera = vsg::Camera::create(projection_matrix, view_matrix);

  m_output_renderer->SetCameraProjectionMatrix(projection_matrix);
  m_output_renderer->SetCameraViewMatrix(view_matrix);

#ifndef NDEBUG
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
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
  job_system->KickJob(job);

  auto camera_move_job = std::make_shared<TimerJob>(
      [camera_info = std::weak_ptr<CameraInfo>(m_camera_info)](
          jobsystem::JobContext *context) {
        if (camera_info.expired()) {
          return JobContinuation::DISPOSE;
        }
        auto time = std::chrono::high_resolution_clock::now();
        auto duration_in_seconds =
            std::chrono::duration<double>(time.time_since_epoch());
        auto x = duration_in_seconds.count();

        double pos = sin(x);

        camera_info.lock()->position.y = 2.5 + pos;

        return JobContinuation::REQUEUE;
      },
      "test-camera-mover", 1s / 20, MAIN);

  job_system->KickJob(camera_move_job);

#endif
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

  std::unique_lock lock(m_image_buffers_and_tiling_mutex);

  auto job_system =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();

  auto service_registry =
      m_subsystems.lock()->RequireSubsystem<services::IServiceRegistry>();

  auto fut_caller = service_registry->Find("render");
  job_system->WaitForCompletion(fut_caller);

  auto opt_caller = fut_caller.get();
  if (!opt_caller.has_value()) {
    LOG_ERR("No render service available")
    return true;
  }

  auto service_caller = opt_caller->get();

  auto counter = std::make_shared<jobsystem::job::JobCounter>();
  for (int i = 0; i < m_current_service_count; i++) {

    Tile current_tile = m_tile_infos[i];

    // build render service request
    graphics::RenderServiceRequest request;
    request.SetExtend({current_tile.width, current_tile.height});
    request.SetCameraData(graphics::CameraData{
        m_camera_info->up, m_camera_info->position, m_camera_info->direction});
    request.SetPerspectiveProjection(graphics::Perspective{0.01, 2000, 30});

    auto rendering_service_request = request.GetRequest();

    auto job = std::make_shared<jobsystem::job::Job>(
        [_this = weak_from_this(), rendering_service_request,
         subsystems = m_subsystems, service_caller,
         i](jobsystem::JobContext *context) {
          auto fut_response = service_caller->Call(rendering_service_request,
                                                   context->GetJobManager());

          context->GetJobManager()->WaitForCompletion(fut_response);
          auto response = fut_response.get();

          if (response->GetStatus() == services::OK) {
            auto color_buffer_encoded =
                std::move(response->GetResult("color").value());

            auto encoding = response->GetResult("encoding").value();
            auto decoder =
                subsystems.lock()
                    ->RequireSubsystem<graphics::IRenderResultEncoder>();

            if (encoding == decoder->GetName()) {
              auto color_buffer =
                  std::move(decoder->Decode(color_buffer_encoded));

              // TODO: read format from image instead assuming it
              auto image_buffer = _this.lock()->m_image_buffers[i];
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
        });
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

void TiledCompositeRenderer::SetServiceCount(int services) {
  UpdateTilingInfo(services);
  UpdateSceneTiling();
}

void TiledCompositeRenderer::UpdateTilingInfo(int tile_count) {
  std::unique_lock lock(m_image_buffers_and_tiling_mutex);
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
      image_buffer = vsg::vec4Array2D::create(tile_width, tile_height);
      image_buffer->properties.dataVariance = vsg::DYNAMIC_DATA;
      image_buffer->properties.format = VK_FORMAT_R8G8B8A8_UNORM;
      m_image_buffers[i] = image_buffer;

      Tile tile{i * tile_width, i * tile_height, tile_width, tile_height, i};
      m_tile_infos[i] = tile;
    }
  }
}

void TiledCompositeRenderer::UpdateSceneTiling() {
  auto scene = std::make_shared<scene::SceneManager>();
  auto root = scene->GetRoot();

  float width, height;
  std::tie(width, height) = GetCurrentSize();

  vsg::Builder builder;
  for (const auto &tile_info : m_tile_infos) {
    vsg::GeometryInfo geomInfo;

    float x = static_cast<float>(tile_info.x) / width;
    float y = static_cast<float>(tile_info.y) / height;
    float dx = static_cast<float>(tile_info.width) / width;
    float dy = static_cast<float>(tile_info.height) / height;

    geomInfo.position = {x, y, 0};
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
m_image_buffers