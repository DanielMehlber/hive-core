#include "common/exceptions/ExceptionsBase.h"
#include "services/ServiceRequest.h"
#include <vsg/all.h>

#ifndef RENDERSERVICEREQUEST_H
#define RENDERSERVICEREQUEST_H

namespace graphics {

/**
 * Conversion between string and vector failed.
 */
DECLARE_EXCEPTION(VectorConversionException);

struct Perspective {
  double near_plane;
  double far_plane;
  double fov;
};

struct Extend {
  int width;
  int height;
};

enum ProjectionType { PERSPECTIVE, ORTHOGRAPHIC };

/**
 * Utility for creating and processing valid rendering service requests.
 */
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

  std::optional<vsg::dmat4> GetViewMatrix() const;
  void SetViewMatrix(std::optional<vsg::dmat4> matrix);

  services::SharedServiceRequest GetRequest() const;
};

} // namespace graphics

#endif // RENDERSERVICEREQUEST_H
