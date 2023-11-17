#include "common/exceptions/ExceptionsBase.h"
#include "services/ServiceRequest.h"
#include <vsg/all.h>

#ifndef RENDERSERVICEREQUEST_H
#define RENDERSERVICEREQUEST_H

namespace graphics {

DECLARE_EXCEPTION(VectorConversionException);

struct Perspective {
  float near_plane;
  float far_plane;
  float fov;
};

struct CameraData {
  vsg::dvec3 camera_up;
  vsg::dvec3 camera_position;
  vsg::dvec3 camera_direction;
};

struct Extend {
  int width;
  int height;
};

enum ProjectionType { PERSPECTIVE, ORTHOGRAPHIC };

class RenderServiceRequest {
private:
  services::SharedServiceRequest m_request;

public:
  explicit RenderServiceRequest(
      services::SharedServiceRequest request =
          std::make_shared<services::ServiceRequest>("render"));

  std::optional<Extend> GetExtend() const;
  void SetExtend(const Extend &extend);

  std::optional<ProjectionType> GetProjectionType() const;
  void SetPerspectiveProjection(const Perspective &perspective);
  std::optional<Perspective> GetPerspectiveProjection() const;

  std::optional<CameraData> GetCameraData() const;
  void SetCameraData(const CameraData &camera_data);

  services::SharedServiceRequest GetRequest() const;
};

} // namespace graphics

#endif // RENDERSERVICEREQUEST_H
