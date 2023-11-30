#include "PeerSetup.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include <gtest/gtest.h>

#ifdef INCLUDE_ENCODER_EVALUATION
#include "EncoderEvaluation.h"
#endif

TEST(GraphicsTests, remote_render_service) {
  auto subsystems_1 = SetupSubsystems();

  auto encoder = std::make_shared<graphics::Base64RenderResultEncoder>();
  subsystems_1->AddOrReplaceSubsystem<graphics::IRenderResultEncoder>(encoder);

  auto job_manager = subsystems_1->RequireSubsystem<JobManager>();

  auto subsystems_2 =
      std::make_shared<common::subsystems::SubsystemManager>(*subsystems_1);

  SharedRenderer renderer = std::make_shared<OffscreenRenderer>();
  subsystems_2->AddOrReplaceSubsystem<IRenderer>(renderer);

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

  std::vector<long> times;
  times.resize(10);
  for (int i = 0; i < 10; i++) {
    auto result_fut =
        caller->Call(GenerateRenderingRequest(1000, 1000), job_manager);

    auto start_point = std::chrono::high_resolution_clock::now();
    job_manager->InvokeCycleAndWait();
    auto end_point = std::chrono::high_resolution_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_point - start_point);

    times[i] = elapsed_time.count();
    LOG_INFO("Remote rendering took " << elapsed_time.count() << "ms");
  }

  auto const count = static_cast<float>(times.size());
  auto average = std::reduce(times.begin(), times.end()) / count;
  LOG_INFO("average remote rendering time was " << average << "ms");
}

TEST(GraphicsTest, offscreen_rendering_sphere) {

  auto subsystems = SetupSubsystems();

  vsg::Builder builder;
  auto scene = std::make_shared<scene::SceneManager>();
  scene->GetRoot()->addChild(builder.createSphere());

  graphics::RendererSetup info;
  SharedRenderer renderer = std::make_shared<OffscreenRenderer>(info, scene);
  subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(renderer);

  SharedServiceExecutor render_service =
      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(subsystems));

  auto job_system = subsystems->RequireSubsystem<JobManager>();

  RenderServiceRequest request_generator;
  request_generator.SetExtend({10, 10});
  auto response =
      render_service->Call(request_generator.GetRequest(), job_system);

  job_system->InvokeCycleAndWait();
  job_system->WaitForCompletion(response);

  auto decoder = subsystems->RequireSubsystem<graphics::IRenderResultEncoder>();
  auto encoded_color_buffer = response.get()->GetResult("color").value();
  auto color_buffer = decoder->Decode(encoded_color_buffer);

  // check that image is not completely transparent
  // format goes like this: r,g,b,a and we want a (every 4th element) with an
  // offset of 4
  int count{0};
  for (int i = 3; i < color_buffer.size(); i += 4) {
    if (color_buffer.at(i) != 0) {
      count++;
    }
  }

  ASSERT_FALSE(count == 0);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}