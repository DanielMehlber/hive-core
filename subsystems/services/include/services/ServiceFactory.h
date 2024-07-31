#pragma once

#include "ServiceRequest.h"
#include "ServiceResponse.h"

namespace services {

/**
 * Builds frequently used instances and objects of the services
 */
class ServiceFactory {
public:
  SharedServiceResponse CreateResponse(const SharedServiceRequest &request);
};

} // namespace services
