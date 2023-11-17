#include "common/test/TryAssertUntilTimeout.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include <gtest/gtest.h>

#define REGISTRY_OF(x) std::get<1>(x)
#define WEB_SOCKET_OF(x) std::get<0>(x)
#define NODE std::tuple<SharedWebSocketPeer, SharedServiceRegistry>

using namespace services;
using namespace networking;
using namespace graphics;
using namespace common::test;

SharedWebSocketPeer
setupPeer(size_t port,
          const common::subsystems::SharedSubsystemManager &subsystems) {

  props::SharedPropertyProvider property_provider =
      std::make_shared<props::PropertyProvider>(subsystems);

  property_provider->Set("net.ws.port", port);

  subsystems->AddOrReplaceSubsystem(property_provider);

  return NetworkingFactory::CreateWebSocketPeer(subsystems);
}

SharedServiceRequest GenerateRenderingRequest(int width, int height) {
  auto request = std::make_shared<ServiceRequest>("render");
  request->SetParameter("width", width);
  request->SetParameter("height", height);
  return request;
}

NODE setupNode(size_t port,
               const common::subsystems::SharedSubsystemManager &subsystems) {
  // setup first peer
  SharedWebSocketPeer web_socket_peer = setupPeer(port, subsystems);

  subsystems->AddOrReplaceSubsystem(web_socket_peer);

  SharedServiceRegistry registry =
      std::make_shared<services::impl::WebSocketServiceRegistry>(subsystems);

  subsystems->AddOrReplaceSubsystem(registry);

  return {web_socket_peer, registry};
}

common::subsystems::SharedSubsystemManager SetupSubsystems() {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();

  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  subsystems->AddOrReplaceSubsystem(job_manager);

  messaging::SharedBroker message_broker =
      messaging::MessagingFactory::CreateBroker(subsystems);

  subsystems->AddOrReplaceSubsystem(message_broker);

  return subsystems;
}

TEST(GraphicsTests, remote_render_service) {
  auto subsystems_1 = SetupSubsystems();

  auto job_manager = subsystems_1->RequireSubsystem<JobManager>();

  auto subsystems_2 =
      std::make_shared<common::subsystems::SubsystemManager>(*subsystems_1);

  SharedRenderer renderer = std::make_shared<OffscreenRenderer>();

  NODE node1 = setupNode(9005, subsystems_1);
  NODE node2 = setupNode(9006, subsystems_2);

  // first establish connection in order to broadcast the connection
  auto connection_progress =
      WEB_SOCKET_OF(node1)->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor render_service =
      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(subsystems_2));

  REGISTRY_OF(node1)->Register(render_service);
  job_manager->InvokeCycleAndWait();

  // wait until rendering job has been registered
  TryAssertUntilTimeout(
      [&node2, job_manager] {
        job_manager->InvokeCycleAndWait();
        return REGISTRY_OF(node2)->Find("render").get().has_value();
      },
      10s);

  SharedServiceCaller caller = REGISTRY_OF(node2)->Find("render").get().value();
  for (int i = 0; i < 3; i++) {
    auto result_fut =
        caller->Call(GenerateRenderingRequest(100, 100), job_manager);

    auto start_point = std::chrono::high_resolution_clock::now();
    job_manager->InvokeCycleAndWait();
    auto end_point = std::chrono::high_resolution_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_point - start_point);

    LOG_INFO("Remote rendering took " << elapsed_time.count() << "ms");

    SharedServiceResponse response;
    ASSERT_NO_THROW(response = result_fut.get());
  }
}

TEST(GraphicsTest, render_service_multiple) {}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}