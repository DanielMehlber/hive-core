#include "PeerSetup.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/impl/PlainRenderResultEncoder.h"
#include <gtest/gtest.h>
#include <numeric>

#ifdef INCLUDE_ENCODER_EVALUATION
#include "EncoderEvaluation.h"
#endif

using namespace common::test;
using namespace jobsystem;

SharedServiceRequest GenerateRenderingRequest(int width, int height) {
  auto request = std::make_shared<ServiceRequest>("render");
  request->SetParameter("width", width);
  request->SetParameter("height", height);
  return request;
}

TEST(GraphicsTests, remote_render_service) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<jobsystem::JobManager>(config);
  job_manager->StartExecution();

  auto peer_1 = setupNode(job_manager, config, 9005);
  auto peer_1_networking_subsystem =
      peer_1->RequireSubsystem<IMessageEndpoint>();
  auto peer_1_service_registry = peer_1->RequireSubsystem<IServiceRegistry>();

  auto peer_2 = setupNode(job_manager, config, 9006);
  auto peer_2_service_registry = peer_2->RequireSubsystem<IServiceRegistry>();

  auto encoder = std::make_shared<graphics::PlainRenderResultEncoder>();
  peer_1->AddOrReplaceSubsystem<graphics::IRenderResultEncoder>(encoder);
  peer_2->AddOrReplaceSubsystem<graphics::IRenderResultEncoder>(encoder);

  SharedRenderer renderer = std::make_shared<OffscreenRenderer>();
  peer_2->AddOrReplaceSubsystem<IRenderer>(renderer);

  // first establish connection in order to broadcast the connection
  auto connection_progress =
      peer_1_networking_subsystem->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor render_service =
      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(peer_1));

  peer_1_service_registry->Register(render_service);
  job_manager->InvokeCycleAndWait();

  // wait until rendering job has been registered
  TryAssertUntilTimeout(
      [&peer_2_service_registry, job_manager] {
        job_manager->InvokeCycleAndWait();
        return peer_2_service_registry->Find("render").get().has_value();
      },
      10s);

  SharedServiceCaller caller =
      peer_2_service_registry->Find("render").get().value();

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
    LOG_INFO("Remote rendering took " << elapsed_time.count() << "ms")
  }

  auto const count = static_cast<float>(times.size());
  auto average = std::reduce(times.begin(), times.end()) / count;
  LOG_INFO("average remote rendering time was " << average << "ms")
}

TEST(GraphicsTest, offscreen_rendering_sphere) {

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem<JobManager>(job_manager);

  vsg::Builder builder;
  auto scene = std::make_shared<scene::SceneManager>();
  scene->GetRoot()->addChild(builder.createSphere());

  graphics::RendererSetup info;
  SharedRenderer renderer = std::make_shared<OffscreenRenderer>(info, scene);
  subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(renderer);

  SharedServiceExecutor render_service =
      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(subsystems));

  RenderServiceRequest request_generator;
  request_generator.SetExtend({10, 10});
  auto response =
      render_service->Call(request_generator.GetRequest(), job_manager);

  job_manager->InvokeCycleAndWait();
  job_manager->WaitForCompletion(response);

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