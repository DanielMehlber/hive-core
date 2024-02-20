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

vsg::dmat4 toMat4(const std::string &serialized_vector) {
  // split components
  std::istringstream iss(serialized_vector);
  std::vector<std::string> components;
  std::string token;
  while (std::getline(iss, token, ' ')) {
    components.push_back(token);
  }

  if (components.size() != 16) {
    THROW_EXCEPTION(VectorConversionException,
                    "string provides " << components.size()
                                       << " components, but 3 required")
  }

  vsg::dmat4 vec;

  for (int i = 0; i < 16; i++) {
    vec.data()[i] = std::stof(components.at(i));
  }

  return vec;
}

std::string toString(const vsg::dvec3 &vec) {
  std::stringstream ss;
  ss << vec.x << " " << vec.y << " " << vec.z;
  return ss.str();
}

std::string toString(const vsg::dmat4 &mat) {
  std::stringstream ss;
  ss << mat.data()[0];
  for (int i = 1; i < 16; i++) {
    ss << " " << mat.data()[i];
  }
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
  auto maybe_width = m_request->GetParameter("width");
  auto maybe_height = m_request->GetParameter("height");

  if (maybe_height.has_value() && maybe_width.has_value()) {
    return Extend{std::stoi(maybe_width.value()),
                  std::stoi(maybe_height.value())};
  }

  return {};
}

std::optional<ProjectionType> RenderServiceRequest::GetProjectionType() const {
  auto maybe_type = m_request->GetParameter("projection-type");
  if (maybe_type.has_value()) {
    auto type = maybe_type.value();
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

void RenderServiceRequest::SetViewMatrix(std::optional<vsg::dmat4> matrix) {
  if (matrix.has_value()) {
    m_request->SetParameter("view-matrix", toString(matrix.value()));
  }
}

std::optional<vsg::dmat4> RenderServiceRequest::GetViewMatrix() const {
  auto param = m_request->GetParameter("view-matrix");
  if (param.has_value()) {
    return toMat4(param.value());
  } else {
    return {};
  }
}
