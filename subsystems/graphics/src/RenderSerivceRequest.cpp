#include "graphics/service/RenderServiceRequest.h"

using namespace graphics;

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

  for (int i = 0; i < 3; i++) {
    vec.data()[i] = std::stof(components.at(i));
  }

  return vec;
}

std::string toString(const vsg::dvec3 &vec) {
  std::stringstream ss;
  ss << vec.x << " " << vec.y << " " << vec.z;
  return ss.str();
}

RenderServiceRequest::RenderServiceRequest(
    services::SharedServiceRequest request)
    : m_request(std::move(request)) {}

void RenderServiceRequest::SetExtend(const Extend &extend) {
  m_request->SetParameter("width", extend.width);
  m_request->SetParameter("height", extend.height);
}

std::optional<Extend> RenderServiceRequest::GetExtend() const {
  auto opt_width = m_request->GetParameter("width");
  auto opt_height = m_request->GetParameter("height");

  if (opt_height.has_value() && opt_width.has_value()) {
    return Extend{std::stoi(opt_width.value()), std::stoi(opt_height.value())};
  }

  return {};
}

std::optional<CameraData> RenderServiceRequest::GetCameraData() const {
  auto opt_camera_pos = m_request->GetParameter("camera-pos");
  auto opt_camera_direction = m_request->GetParameter("camera-direction");
  auto opt_camera_up = m_request->GetParameter("camera-up");

  if (opt_camera_up.has_value() || opt_camera_direction.has_value() ||
      opt_camera_pos.has_value()) {
    vsg::dvec3 eye = toVec3(opt_camera_pos.value_or("0.0 0.0 0.0"));
    vsg::dvec3 direction = toVec3(opt_camera_direction.value_or("1.0 0.0 0.0"));
    vsg::dvec3 up = toVec3(opt_camera_up.value_or("0.0 0.0 1.0"));

    return CameraData{up, eye, direction};
  }

  return {};
}

void RenderServiceRequest::SetCameraData(const CameraData &camera_data) {
  m_request->SetParameter("camera-pos", toString(camera_data.camera_position));
  m_request->SetParameter("camera-direction",
                          toString(camera_data.camera_direction));
  m_request->SetParameter("camera-up", toString(camera_data.camera_up));
}

std::optional<ProjectionType> RenderServiceRequest::GetProjectionType() const {
  auto opt_type = m_request->GetParameter("projection-type");
  if (opt_type.has_value()) {
    auto type = opt_type.value();
    if (type == "perspective") {
      return ProjectionType::PERSPECTIVE;
    } else if (type == "orthographic") {
      return ProjectionType::ORTHOGRAPHIC;
    }
  }

  return {};
}

void RenderServiceRequest::SetPerspectiveProjection(
    const Perspective &perspective) {
  m_request->SetParameter<std::string>("projection-type", "perspective");
  m_request->SetParameter("perspective-near-plane", perspective.near_plane);
  m_request->SetParameter("perspective-far-plane", perspective.far_plane);
  m_request->SetParameter("perspective-fov", perspective.fov);
}

std::optional<Perspective>
RenderServiceRequest::GetPerspectiveProjection() const {
  auto near_plane =
      std::stof(m_request->GetParameter("perspective-near-plane").value());
  auto far_plane =
      std::stof(m_request->GetParameter("perspective-far-plane").value());
  auto fov = std::stof(m_request->GetParameter("perspective-fov").value());

  return Perspective{near_plane, far_plane, fov};
}

services::SharedServiceRequest RenderServiceRequest::GetRequest() const {
  return m_request;
}
