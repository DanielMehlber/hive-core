// TODO: adapt to new API
//#ifndef SIMULATION_FRAMEWORK_ENCODEREVALUATION_H
//#define SIMULATION_FRAMEWORK_ENCODEREVALUATION_H
//
//#include "PeerSetup.h"
//#include "common/test/TryAssertUntilTimeout.h"
//#include "events/EventFactory.h"
//#include "graphics/renderer/impl/OffscreenRenderer.h"
//#include "graphics/service/RenderService.h"
//#include "graphics/service/RenderServiceRequest.h"
//#include "graphics/service/encoders/IRenderResultEncoder.h"
//#include "graphics/service/encoders/impl/Base64RenderResultEncoder.h"
//#include "graphics/service/encoders/impl/GzipRenderResultEncoder.h"
//#include "graphics/service/encoders/impl/PlainRenderResultEncoder.h"
//#include "networking/NetworkingFactory.h"
//#include "services/registry/impl/remote/RemoteServiceRegistry.h"
//#include <gtest/gtest.h>
//
//using namespace common::test;
//
//struct Measurement {
//  int image_size;
//  float time_ms;
//};
//
//struct Encoder {
//  std::string name;
//  std::vector<Measurement> measurements;
//};
//
//struct Evaluation {
//  std::vector<Encoder> encoders;
//};
//
//TEST(GraphicsTests, encoder_eval) {
//  auto subsystems_1 = SetupSubsystems();
//
//  auto job_manager = subsystems_1->RequireSubsystem<JobManager>();
//
//  auto subsystems_2 =
//      std::make_shared<common::subsystems::SubsystemManager>(*subsystems_1);
//
//  SharedRenderer renderer = std::make_shared<OffscreenRenderer>();
//  subsystems_2->AddOrReplaceSubsystem<IRenderer>(renderer);
//
//  NODE node1 = setupNode(9005, subsystems_1);
//  NODE node2 = setupNode(9006, subsystems_2);
//
//  // first establish connection in order to broadcast the connection
//  auto connection_progress =
//      WEB_SOCKET_OF(node1)->EstablishConnectionTo("127.0.0.1:9006");
//
//  connection_progress.wait();
//  ASSERT_NO_THROW(connection_progress.get());
//
//  SharedServiceExecutor render_service =
//      std::static_pointer_cast<services::impl::LocalServiceExecutor>(
//          std::make_shared<RenderService>(subsystems_2));
//
//  REGISTRY_OF(node1)->Register(render_service);
//  job_manager->InvokeCycleAndWait();
//
//  // wait until rendering job has been registered
//  TryAssertUntilTimeout(
//      [&node2, job_manager] {
//        job_manager->InvokeCycleAndWait();
//        return REGISTRY_OF(node2)->Find("render").get().has_value();
//      },
//      10s);
//
//  SharedServiceCaller caller = REGISTRY_OF(node2)->Find("render").get().value();
//
//  std::shared_ptr<IRenderResultEncoder> encoders[]{
//      std::make_shared<PlainRenderResultEncoder>(),
//      std::make_shared<Base64RenderResultEncoder>(),
//      std::make_shared<GzipRenderResultEncoder>(),
//  };
//
//  Evaluation eval;
//
//  for (const auto &encoder : encoders) {
//    Encoder encoder_eval;
//    encoder_eval.name = encoder->GetName();
//
//    subsystems_1->AddOrReplaceSubsystem<IRenderResultEncoder>(encoder);
//    subsystems_2->AddOrReplaceSubsystem<IRenderResultEncoder>(encoder);
//
//    for (int image_size = 100; image_size <= 1500; image_size += 200) {
//
//      std::vector<long> times;
//      times.resize(5);
//      for (int i = 0; i < 5; i++) {
//        auto result_fut = caller->Call(
//            GenerateRenderingRequest(image_size, image_size), job_manager);
//
//        auto start_point = std::chrono::high_resolution_clock::now();
//        job_manager->InvokeCycleAndWait();
//        SharedServiceResponse response;
//        ASSERT_NO_THROW(response = result_fut.get());
//        ASSERT_TRUE(response->GetStatus() == OK);
//
//        auto encoded_result = response->GetResult("color").value();
//        auto decoded_result = encoder->Decode(encoded_result);
//        auto end_point = std::chrono::high_resolution_clock::now();
//
//        auto elapsed_time =
//            std::chrono::duration_cast<std::chrono::milliseconds>(end_point -
//                                                                  start_point);
//
//        times[i] = elapsed_time.count();
//      }
//
//      auto const count = static_cast<float>(times.size());
//      auto average = std::reduce(times.begin(), times.end()) / count;
//      LOG_INFO("average remote rendering time was "
//               << average << "ms for image size " << image_size
//               << " using encoder " << encoder->GetName());
//
//      Measurement measurement{image_size, average};
//      encoder_eval.measurements.push_back(measurement);
//    }
//
//    eval.encoders.push_back(encoder_eval);
//  }
//
//  // convert evaluation
//  std::stringstream ss;
//
//  // write header
//  ss << "Encoder";
//  Encoder first = eval.encoders.at(0);
//  for (auto measurement : first.measurements) {
//    ss << "," << measurement.image_size << "x" << measurement.image_size;
//  }
//  ss << "\n";
//
//  // write encoders
//  for (auto encoder_eval : eval.encoders) {
//    ss << encoder_eval.name;
//    for (auto measurement : encoder_eval.measurements) {
//      ss << "," << measurement.time_ms;
//    }
//    ss << "\n";
//  }
//
//  std::string csv = ss.str();
//
//  // write file to disk
//  // Create an output file stream object
//  std::ofstream outFile;
//
//  // Open the file for writing
//  outFile.open("./eval.csv", std::ios::trunc);
//
//  // Check if the file is opened successfully
//  if (!outFile.is_open()) {
//    LOG_ERR("Error writing to file");
//  }
//
//  // Write the text content to the file
//  outFile << csv;
//
//  // Close the file
//  outFile.close();
//}
//
//#endif // SIMULATION_FRAMEWORK_ENCODEREVALUATION_H
