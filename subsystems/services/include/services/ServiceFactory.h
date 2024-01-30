#ifndef SERVICEFACTORY_H
#define SERVICEFACTORY_H

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

#endif // SERVICEFACTORY_H
