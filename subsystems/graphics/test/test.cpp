#include "PeerSetup.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "graphics/renderer/IRenderer.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/impl/PlainRenderResultEncoder.h"
#include "logging/LogManager.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include <gtest/gtest.h>
#include <numeric>

#ifdef INCLUDE_ENCODER_EVALUATION
#include "EncoderEvaluation.h"
#endif

using namespace hive::common::test;
using namespace hive::jobsystem;
using namespace hive::graphics;

SharedServiceRequest GenerateRenderingRequest(int width, int height) {
  auto request = std::make_shared<ServiceRequest>("render");
  request->SetParameter("width", width);
  request->SetParameter("height", height);
  return request;
}

inline void waitUntilConnectionCompleted(Node &node1, Node &node2) {
  TryAssertUntilTimeout(
      [&node1, &node2]() {
        node2.job_manager.Borrow()->InvokeCycleAndWait();
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        bool node1_connected =
            node1.networking_mgr.Borrow()
                ->GetSomeMessageEndpointConnectedTo(node2.uuid)
                .has_value();
        bool node2_connected =
            node2.networking_mgr.Borrow()
                ->GetSomeMessageEndpointConnectedTo(node1.uuid)
                .has_value();
        return node1_connected && node2_connected;
      },
      5s);
}

TEST(GraphicsTest, remote_render_service) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);
  auto node_2 = setupNode(config, 9006);

  auto encoder = common::memory::Owner<PlainRenderResultEncoder>();
  node_1.subsystems->AddOrReplaceSubsystem<IRenderResultEncoder>(
      common::memory::Owner<PlainRenderResultEncoder>());
  node_2.subsystems->AddOrReplaceSubsystem<IRenderResultEncoder>(
      common::memory::Owner<PlainRenderResultEncoder>());

  common::memory::Owner<IRenderer> renderer =
      common::memory::Owner<OffscreenRenderer>();
  node_2.subsystems->AddOrReplaceSubsystem<IRenderer>(std::move(renderer));

  // first establish connection in order to broadcast the connection
  auto connection_progress = node_1.networking_mgr.Borrow()
                                 ->GetPrimaryMessageEndpoint()
                                 .value()
                                 ->EstablishConnectionTo("127.0.0.1:9006");

  waitUntilConnectionCompleted(node_1, node_2);
  ASSERT_NO_THROW(connection_progress.get());

  // process established connection
  node_1.job_manager.Borrow()->InvokeCycleAndWait();
  node_2.job_manager.Borrow()->InvokeCycleAndWait();

  SharedServiceExecutor render_service =
      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(node_1.subsystems.CreateReference()));

  node_1.registry.Borrow()->Register(render_service);
  node_1.job_manager.Borrow()->InvokeCycleAndWait();
  node_2.job_manager.Borrow()->InvokeCycleAndWait();

  // wait until rendering job has been registered
  TryAssertUntilTimeout(
      [&node_1, &node_2] {
        node_1.job_manager.Borrow()->InvokeCycleAndWait();
        node_2.job_manager.Borrow()->InvokeCycleAndWait();
        return node_2.registry.Borrow()->Find("render").get().has_value();
      },
      10s);

  SharedServiceCaller caller =
      node_2.registry.Borrow()->Find("render").get().value();

  std::vector<long> times;
  times.resize(10);
  for (int i = 0; i < 10; i++) {
    auto result_fut = caller->IssueCallAsJob(
        GenerateRenderingRequest(1000, 1000), node_2.job_manager.Borrow());

    auto start_point = std::chrono::high_resolution_clock::now();

    // we need a second thread here: while the requesting node waits, the
    // processing node must resolve its service request. This can't be done in
    // the same thread sequentially, but only in parallel.
    std::atomic_bool finished = false;
    auto request_processing_thread = std::thread([&node_1, &finished]() {
      while (!finished.load()) {
        node_1.job_manager.Borrow()->InvokeCycleAndWait();
      }
    });

    node_2.job_manager.Borrow()->InvokeCycleAndWait();
    finished = true;
    request_processing_thread.join();

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

  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  auto config = std::make_shared<common::config::Configuration>();

  auto job_manager = common::memory::Owner<JobManager>(config);
  auto job_manager_ref = job_manager.CreateReference();
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  vsg::Builder builder;
  auto scene = std::make_shared<scene::SceneManager>();
  scene->GetRoot()->addChild(builder.createSphere());

  RendererSetup info;
  auto renderer = common::memory::Owner<OffscreenRenderer>(info, scene);
  subsystems->AddOrReplaceSubsystem<IRenderer>(std::move(renderer));

  SharedServiceExecutor render_service =
      std::static_pointer_cast<impl::LocalServiceExecutor>(
          std::make_shared<RenderService>(subsystems.CreateReference()));

  RenderServiceRequest request_generator;
  request_generator.SetExtend({10, 10});
  auto response = render_service->IssueCallAsJob(request_generator.GetRequest(),
                                                 job_manager_ref.Borrow());

  job_manager_ref.Borrow()->InvokeCycleAndWait();
  job_manager_ref.Borrow()->Await(response);

  auto decoder = subsystems->RequireSubsystem<IRenderResultEncoder>();
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