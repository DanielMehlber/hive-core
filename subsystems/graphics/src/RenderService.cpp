#include "graphics/service/RenderService.h"
#include "common/profiling/Timer.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/SimpleViewMatrix.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "graphics/service/encoders/impl/PlainRenderResultEncoder.h"
#include "logging/LogManager.h"

using namespace hive;
using namespace hive::graphics;
using namespace std::placeholders;
using namespace services;

DECLARE_EXCEPTION(MatrixConversionException);
DECLARE_EXCEPTION(VectorConversionException);

RenderService::RenderService(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    common::memory::Reference<graphics::IRenderer> renderer)
    : services::impl::LocalServiceExecutor(
          "render", std::bind(&RenderService::Render, this, _1)),
      m_subsystems(subsystems), m_renderer(std::move(renderer)) {

  // register render-result encoder if none
  if (!m_subsystems.Borrow()->ProvidesSubsystem<IRenderResultEncoder>()) {
    m_subsystems.Borrow()->AddOrReplaceSubsystem<IRenderResultEncoder>(
        common::memory::Owner<PlainRenderResultEncoder>());
  }
}

std::future<SharedServiceResponse>
RenderService::Render(const services::SharedServiceRequest &raw_request) {
  // only process a single render process at a time.
  std::unique_lock lock(m_service_mutex);

  std::promise<SharedServiceResponse> promise;
  std::future<SharedServiceResponse> future = promise.get_future();

  RenderServiceRequest request(raw_request);

  auto extend = request.GetExtend().value();
  auto maybe_projection_type = request.GetProjectionType();
  auto maybe_view_matrix = request.GetViewMatrix();

  auto response =
      std::make_shared<ServiceResponse>(raw_request->GetTransactionId());

  auto maybe_rendering_subsystem = GetRenderer();
  bool rendering_subsystem_available = maybe_rendering_subsystem.has_value();
  if (!rendering_subsystem_available) {
    LOG_ERR("Cannot perform rendering service because there is no renderer "
            "available")
    response->SetStatusMessage("Cannot perform rendering service because there "
                               "is no renderer available");
    response->SetStatus(INTERNAL_ERROR);
    promise.set_value(response);
    return future;
  }

  auto rendering_subsystem = maybe_rendering_subsystem.value();

  // check if renderer must be resized
  const auto [current_width, current_height] =
      rendering_subsystem->GetCurrentSize();
  bool size_changed =
      current_height != extend.height || current_width != extend.width;
  if (size_changed) {
    rendering_subsystem->Resize(extend.width, extend.height);
  }

#ifdef ENABLE_PROFILING
  // setup timers (for profiling purposes)
  common::profiling::Timer rendering_timer("rendering-service-rendering-" +
                                           std::to_string(current_width) + "x" +
                                           std::to_string(current_height));
  common::profiling::Timer complete_timer("rendering-service-complete" +
                                          std::to_string(current_width) + "x" +
                                          std::to_string(current_height));
#endif

  try {
    // apply settings to renderer's main camera
    if (maybe_projection_type.has_value()) {
      auto projection_matrix_type = maybe_projection_type.value();

      // check which projection (perspective, orthographic) is requested
      if (projection_matrix_type == ProjectionType::PERSPECTIVE) {
        // required values for perspective projection setting

        auto perspective = request.GetPerspectiveProjection().value();
        auto ratio = static_cast<double>(extend.width) /
                     static_cast<double>(extend.height);
        auto matrix = vsg::Perspective::create(perspective.fov, ratio,
                                               perspective.near_plane,
                                               perspective.far_plane);
        rendering_subsystem->SetCameraProjectionMatrix(matrix);
      } else if (projection_matrix_type == ProjectionType::ORTHOGRAPHIC) {
        // TODO: Support orthographic projection
        LOG_WARN("orthographic projection in render service is not yet "
                 "implemented")
      }
    }
  } catch (const std::exception &exception) {
    LOG_ERR("cannot execute rendering process due to invalid parameters: "
            << exception.what())

    response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
    response->SetStatusMessage(exception.what());
    promise.set_value(response);
    return future;
  }

  // check if camera must be moved
  bool camera_view_set = maybe_view_matrix.has_value();
  if (camera_view_set) {
    auto view_matrix = maybe_view_matrix.value();
    auto simple_view_matrix = graphics::SimpleViewMatrix::create(view_matrix);
    rendering_subsystem->SetCameraViewMatrix(simple_view_matrix);
  }

  rendering_subsystem->Render();

#ifdef ENABLE_PROFILING
  // do not count encoding of bytes etc.
  rendering_timer.Stop();
#endif

  auto maybe_result = rendering_subsystem->GetResult();
  if (!maybe_result.has_value()) {
    LOG_ERR(
        "renderer used for render service did not return any rending result")
    response->SetStatus(ServiceResponseStatus::INTERNAL_ERROR);
    response->SetStatusMessage("renderer did not return any rendering result");
    promise.set_value(response);
    return future;
  }

  auto result = maybe_result.value();

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    // attach color render result to response
    auto encoder = subsystems->RequireSubsystem<IRenderResultEncoder>();
    response->SetResult("encoding", encoder->GetName());

    auto color_data = result->ExtractColorData();

    auto encoded_color = encoder->Encode(color_data);
    response->SetResult("color", std::move(encoded_color));

    // check if depth buffer is requested as well and attach it accordingly
    bool depth_requested = std::stoi(
        raw_request->GetParameter("include-depth-buffer").value_or("0"));
    if (depth_requested) {
      auto depth_data = result->ExtractDepthData();
      auto encoded_depth = encoder->Encode(depth_data);
      response->SetResult("depth", std::move(encoded_depth));
    }

    promise.set_value(response);
    return future;
  } else /* if subsystems are not available */ {
    LOG_ERR("cannot encode render service result because subsystems are not "
            "available")
    response->SetStatus(ServiceResponseStatus::INTERNAL_ERROR);
    response->SetStatusMessage("no encoder found for result");
    promise.set_value(response);
    return future;
  }
}

std::optional<common::memory::Borrower<IRenderer>>
RenderService::GetRenderer() {
  if (m_renderer) {
    return m_renderer.TryBorrow();
  } else {
    if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
      auto subsystems = maybe_subsystems.value();
      return subsystems->GetSubsystem<graphics::IRenderer>();
    } else {
      LOG_WARN("no renderer found in render service because subsystems are not "
               "available")
      return {};
    }
  }
}
