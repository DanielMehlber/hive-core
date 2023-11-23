#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "graphics/service/encoders/impl/Base64RenderResultEncoder.h"
#include "graphics/service/encoders/impl/CharEscapeRenderResultEncoder.h"
#include "graphics/service/encoders/impl/PlainRenderResultEncoder.h"
#include <boost/archive/iterators/transform_width.hpp>

using namespace graphics;
using namespace std::placeholders;

DECLARE_EXCEPTION(MatrixConversionException);
DECLARE_EXCEPTION(VectorConversionException);

RenderService::RenderService(
    const common::subsystems::SharedSubsystemManager &subsystems,
    std::shared_ptr<graphics::IRenderer> renderer)
    : services::impl::LocalServiceExecutor(
          "render", std::bind(&RenderService::Render, this, _1)),
      m_subsystems(subsystems), m_renderer(std::move(renderer)) {

  // register render-result encoder if none
  if (!m_subsystems.lock()->ProvidesSubsystem<IRenderResultEncoder>()) {
    m_subsystems.lock()->AddOrReplaceSubsystem<IRenderResultEncoder>(
        std::make_shared<PlainRenderResultEncoder>());
  }
}

std::future<services::SharedServiceResponse>
RenderService::Render(const services::SharedServiceRequest &raw_request) {
  std::unique_lock lock(m_service_mutex);

  std::promise<SharedServiceResponse> promise;
  std::future<SharedServiceResponse> future = promise.get_future();

  RenderServiceRequest request(raw_request);

  auto extend = request.GetExtend().value();
  auto opt_projection_type = request.GetProjectionType();
  auto opt_camera = request.GetCameraData();

  auto response =
      std::make_shared<ServiceResponse>(raw_request->GetTransactionId());

  auto opt_rendering_subsystem = GetRenderer();
  bool rendering_subsystem_available = opt_rendering_subsystem.has_value();
  if (!rendering_subsystem_available) {
    LOG_ERR("Cannot perform rendering service because there is no renderer "
            "available")
    response->SetStatusMessage("Cannot perform rendering service because there "
                               "is no renderer available");
    response->SetStatus(INTERNAL_ERROR);
    promise.set_value(response);
    return future;
  }

  auto rendering_subsystem = opt_rendering_subsystem.value();

  try {

    // apply settings to renderer's main camera
    if (opt_projection_type.has_value()) {
      auto projection_matrix_type = opt_projection_type.value();

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
        LOG_WARN("Orthographic projection in Render-Service is not yet "
                 "implemented")
      }
    }
  } catch (const std::exception &exception) {
    LOG_ERR("Cannot execute rendering process due to invalid parameters: "
            << exception.what())

    response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
    response->SetStatusMessage(exception.what());
    promise.set_value(response);
    return future;
  }

  // check if renderer must be resized
  const auto [current_width, current_height] =
      rendering_subsystem->GetCurrentSize();
  bool size_changed =
      current_height != extend.height || current_width != extend.width;
  if (size_changed) {
    rendering_subsystem->Resize(extend.width, extend.height);
  }

  // check if camera must be moved
  bool camera_view_set = opt_camera.has_value();
  if (camera_view_set) {
    auto camera_info = opt_camera.value();
    vsg::dvec3 eye = camera_info.camera_position;
    vsg::dvec3 direction = camera_info.camera_direction;
    vsg::dvec3 up = camera_info.camera_up;

    auto look_at = vsg::LookAt::create(eye, direction, up);
    rendering_subsystem->SetCameraViewMatrix(look_at);
  }

  rendering_subsystem->Render();

  auto opt_result = rendering_subsystem->GetResult();

  if (!opt_result.has_value()) {
    LOG_ERR(
        "Renderer used for Render-Service did not return any rending result")
    response->SetStatus(ServiceResponseStatus::INTERNAL_ERROR);
    response->SetStatusMessage("Renderer did not return any rendering result");
    promise.set_value(response);
    return future;
  }

  auto result = opt_result.value();

  // attach color render result to response
  auto encoder = m_subsystems.lock()->RequireSubsystem<IRenderResultEncoder>();
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
}

std::optional<graphics::SharedRenderer> RenderService::GetRenderer() const {
  if (m_renderer) {
    return m_renderer;
  } else {
    return m_subsystems.lock()->GetSubsystem<graphics::IRenderer>();
  }
}
