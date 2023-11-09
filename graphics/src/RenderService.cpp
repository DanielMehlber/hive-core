#include "graphics/service/RenderService.h"
#include "common/exceptions/ExceptionsBase.h"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/serialization/split_free.hpp>

using namespace graphics;
using namespace std::placeholders;

DECLARE_EXCEPTION(MatrixConversionException);
DECLARE_EXCEPTION(VectorConversionException);

vsg::mat4 to4x4Matrix(const std::string &serialized_matrix) {
  // split components
  std::istringstream iss(serialized_matrix);
  std::vector<std::string> components;
  std::string token;
  while (std::getline(iss, token, ' ')) {
    components.push_back(token);
  }

  if (components.size() != 16) {
    THROW_EXCEPTION(MatrixConversionException,
                    "string provides " << components.size()
                                       << " components, but 16 required")
  }

  vsg::mat4 matrix;
  for (int i = 0; i < 16; i++) {
    matrix.data()[i] = std::stof(components.at(i));
  }

  return matrix;
}

vsg::dvec3 toVec3(const std::string &serialized_vector) {
  // split components
  std::istringstream iss(serialized_vector);
  std::vector<std::string> components;
  std::string token;
  while (std::getline(iss, token, ' ')) {
    components.push_back(token);
  }

  if (components.size() != 3) {
    THROW_EXCEPTION(VectorConversionException,
                    "string provides " << components.size()
                                       << " components, but 3 required")
  }

  vsg::dvec3 vec;

  for (int i = 0; i < 16; i++) {
    vec.data()[i] = std::stof(components.at(i));
  }

  return vec;
}

std::string imageBytesToString(const std::vector<unsigned char> &buffer) {
  using namespace boost::archive::iterators;

  typedef base64_from_binary< // Binary-to-base64 encoding
      transform_width<std::vector<unsigned char>::const_iterator, 6, 8>>
      base64_encoder;

  // Encode the input data to base64.
  return std::string(base64_encoder(buffer.begin()),
                     base64_encoder(buffer.end()));
}

RenderService::RenderService(SharedRenderer renderer)
    : services::impl::LocalServiceExecutor(
          "render", std::bind(&RenderService::Render, this, _1)),
      m_renderer(std::move(renderer)) {}

std::future<services::SharedServiceResponse>
RenderService::Render(services::SharedServiceRequest request) {

  std::promise<SharedServiceResponse> promise;
  std::future<SharedServiceResponse> future = promise.get_future();

  auto opt_width = request->GetParameter("width");
  auto opt_height = request->GetParameter("height");
  auto opt_projection_type = request->GetParameter("projection-type");
  auto opt_camera_pos = request->GetParameter("camera-pos");
  auto opt_camera_direction = request->GetParameter("camera-direction");
  auto opt_camera_up = request->GetParameter("camera-up");

  auto response =
      std::make_shared<ServiceResponse>(request->GetTransactionId());

  // extract parameters
  int width, height;
  vsg::mat4 projection, view;
  try {
    width = std::stoi(opt_width.value());
    height = std::stoi(opt_height.value());

    // apply settings to renderer's main camera
    if (opt_projection_type.has_value()) {
      auto projection_matrix_type = opt_projection_type.value();

      // check which projection (perspective, orthographic) is requested
      if (projection_matrix_type == "perspective") {
        // required values for perspective projection setting
        auto near_plane =
            std::stof(request->GetParameter("perspective-near-plane").value());
        auto far_plane =
            std::stof(request->GetParameter("perspective-far-plane").value());
        auto fov = std::stof(request->GetParameter("perspective-fov").value());
        auto ratio = static_cast<double>(width) / static_cast<double>(height);
        auto matrix =
            vsg::Perspective::create(fov, ratio, near_plane, far_plane);
        m_renderer->SetCameraProjectionMatrix(matrix);
      } else if (projection_matrix_type == "orthographic") {
        // TODO: Support orthographic projection
        LOG_WARN("Orthographic projection in Render-Service is not yet "
                 "implemented");
      }
    }
  } catch (const std::exception &exception) {
    LOG_ERR("Cannot execute rendering process due to invalid parameters: "
            << exception.what());

    response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
    response->SetStatusMessage(exception.what());
    promise.set_value(response);
    return future;
  }

  // check if renderer must be resized
  const auto [current_width, current_height] = m_renderer->GetCurrentSize();
  bool size_changed = current_height != height || current_width != width;
  if (size_changed) {
    m_renderer->Resize(width, height);
  }

  // check if camera must be moved
  bool camera_view_set = opt_camera_direction.has_value() ||
                         opt_camera_pos.has_value() ||
                         opt_camera_up.has_value();
  if (camera_view_set) {
    vsg::dvec3 eye = toVec3(opt_camera_pos.value_or("0.0 0.0 0.0"));
    vsg::dvec3 direction = toVec3(opt_camera_direction.value_or("1.0 0.0 0.0"));
    vsg::dvec3 up = toVec3(opt_camera_up.value_or("0.0 0.0 1.0"));

    auto look_at = vsg::LookAt::create(eye, direction, up);
    m_renderer->SetCameraViewMatrix(look_at);
  }

  m_renderer->Render();

  auto opt_result = m_renderer->GetResult();

  if (!opt_result.has_value()) {
    LOG_ERR(
        "Renderer used for Render-Service did not return any rending result");
    response->SetStatus(ServiceResponseStatus::INTERNAL_ERROR);
    response->SetStatusMessage("Renderer did not return any rendering result");
    promise.set_value(response);
    return future;
  }

  auto result = opt_result.value();

  // attach color render result to response
  std::string rendered_color_image_base_64 =
      imageBytesToString(result->ExtractColorData());
  response->SetResult("color", std::move(rendered_color_image_base_64));

  // check if depth buffer is requested as well and attach it accordingly
  bool depth_requested =
      std::stoi(request->GetParameter("include-depth-buffer").value_or("1"));
  if (depth_requested) {
    std::string rendered_depth_image_base_64 =
        imageBytesToString(result->ExtractDepthData());
    response->SetResult("depth", std::move(rendered_depth_image_base_64));
  }

  promise.set_value(response);
  return future;
}
