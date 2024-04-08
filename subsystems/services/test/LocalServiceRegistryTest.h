#ifndef LOCALSERVICEREGISTRYTEST_H
#define LOCALSERVICEREGISTRYTEST_H

#include "AddingServiceExecutor.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include <gtest/gtest.h>

using namespace services;
using namespace std::placeholders;

TEST(ServiceTests, adding_service) {
  auto config = std::make_shared<common::config::Configuration>();

  common::memory::Owner<services::IServiceRegistry> registry =
      common::memory::Owner<services::impl::LocalOnlyServiceRegistry>();

  SharedServiceExecutor adding_service =
      std::make_shared<AddingServiceExecutor>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);

  job_manager->StartExecution();
  registry->Register(adding_service);

  SharedServiceCaller service_caller = registry->Find("add").get().value();

  SharedServiceRequest request = GenerateAddingRequest(6, 5);

  auto result_fut = service_caller->IssueCallAsJob(request, job_manager.Borrow());
  job_manager->InvokeCycleAndWait();

  result_fut.wait();
  auto result = result_fut.get();
  ASSERT_EQ(11, GetResultOfAddition(result));

  job_manager->StopExecution();
}

#endif // LOCALSERVICEREGISTRYTEST_H
