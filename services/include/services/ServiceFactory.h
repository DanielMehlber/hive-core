#ifndef SERVICEFACTORY_H
#define SERVICEFACTORY_H

#include "ServiceRequest.h"
#include "ServiceResponse.h"
#include "Services.h"

namespace services {

/**
 * @brief Builds frequently used instances and objects of the services
 */
class SERVICES_API ServiceFactory {
public:
  SharedServiceResponse CreateResponse(const SharedServiceRequest &request);
};

} // namespace services

#endif // SERVICEFACTORY_H
