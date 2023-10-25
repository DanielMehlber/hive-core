#ifndef LOCALSERVICEREGISTRYTEST_H
#define LOCALSERVICEREGISTRYTEST_H

#include "AddingServiceExecutor.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include <gtest/gtest.h>

using namespace services;
using namespace std::placeholders;

TEST(ServiceTests, adding_service) {
  SharedServiceRegistry registry =
      std::make_shared<services::impl::LocalOnlyServiceRegistry>();

  SharedServiceExecutor adding_service =
      std::make_shared<AddingServiceExecutor>();
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();

  registry->Register(adding_service);

  SharedServiceCaller service_caller = registry->Find("add").get().value();

  SharedServiceRequest request = GenerateAddingRequest(6, 5);

  auto result_fut = service_caller->Call(request, job_manager);
  job_manager->InvokeCycleAndWait();

  result_fut.wait();
  auto result = result_fut.get();
  ASSERT_EQ(11, GetResultOfAddition(result));
}

#endif // LOCALSERVICEREGISTRYTEST_H
