#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/variables_map.hpp"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/renderer/impl/OnscreenRenderer.h"
#include "kernel/Kernel.h"
#include <memory>
#include <vector>

struct EvaluationSettings {
  int min_node_count{0};
  int max_node_count{5};
  int node_step_size{1};
  int min_image_size{200};
  int max_image_size{1500};
  int image_step_size{100};
  int coordinator_port{9000};
  std::string plugin_path{"<empty>"};
};

std::optional<std::shared_ptr<scene::SceneManager>> LoadScene() {
  auto vsg_options = vsg::Options::create();
  vsg_options->sharedObjects = vsg::SharedObjects::create();
  vsg_options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
  vsg_options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

  auto scene = std::make_shared<scene::SceneManager>();

  std::array<std::string, 2> scene_objects_to_load = {
      "./data/models/teapot.vsgt", "./data/models/skybox.vsgt"};

  for (const auto &scene_object_file_path : scene_objects_to_load) {
    auto node = vsg::read_cast<vsg::Node>(scene_object_file_path, vsg_options);
    if (node) {
      scene->GetRoot()->addChild(node);
    } else {
      LOG_ERR("Cannot load scene " << scene_object_file_path)
      return {};
    }
  }

  auto light = vsg::DirectionalLight::create();
  light->color = {1, 1, 1};
  light->intensity = 20;
  light->direction = {-3, -3, -3};
  scene->GetRoot()->addChild(light);

  return scene;
}

std::unique_ptr<kernel::Kernel>
CreateCoordinator(const EvaluationSettings &settings,
                  const std::shared_ptr<scene::SceneManager> scene) {

  /* CREATE CONFIGURATION FOR COORDINATOR */
  auto coordinator_config = std::make_shared<common::config::Configuration>();
  coordinator_config->Set("net.port", settings.coordinator_port);

  auto coordinator =
      std::make_unique<kernel::Kernel>(coordinator_config, false);

  /* SETUP ONSCREEN RENDERER */
  auto renderer = std::make_shared<graphics::OnscreenRenderer>(
      scene, settings.min_image_size, settings.min_image_size);
  coordinator->SetRenderer(renderer);
  coordinator->EnableRenderingJob();

  /* SETUP OFFSCREEN RENDERER FOR OWN RENDERING SERVICE */
  auto offscreen_renderer = std::make_shared<graphics::OffscreenRenderer>(
      renderer->GetSetup(), scene);
  coordinator->EnableRenderingService(offscreen_renderer);

  /* LOAD PLUGIN FOR TILED RENDERING */
  coordinator->GetPluginManager()->InstallPlugin(settings.plugin_path);

  return coordinator;
}

std::unique_ptr<kernel::Kernel>
StartWorker(const EvaluationSettings &settings,
            const std::shared_ptr<scene::SceneManager> &scene, int port) {

  /* CREATE CONFIGURATION FOR WORKER */
  auto worker_config = std::make_shared<common::config::Configuration>();
  worker_config->Set("net.port", port);

  auto worker = std::make_unique<kernel::Kernel>(worker_config, false);

  /* SETUP OFFSCREEN RENDERER FOR OWN RENDERING SERVICE */
  graphics::RendererSetup setup = {};
  auto offscreen_renderer =
      std::make_shared<graphics::OffscreenRenderer>(setup, scene);
  worker->EnableRenderingService(offscreen_renderer);
  worker->SetRenderer(offscreen_renderer);

  return worker;
}

namespace po = boost::program_options;

EvaluationSettings ParseSettings(int argc, char **argv) {
  EvaluationSettings settings;

  po::options_description options("Allowed options");
  options.add_options()("help", "show this help message")(
      "plugin", po::value<std::string>(&settings.plugin_path),
      "path to plugin directory");

  return settings;
};

int main(int argc, char **argv) {
  EvaluationSettings settings = ParseSettings(argc, argv);

  /* LOAD SCENE FOR TESTING PURPOSES */
  auto scene = LoadScene().value();

  for (int node_count = settings.min_node_count;
       node_count < settings.max_node_count;
       node_count += settings.node_step_size) {

    std::vector<std::unique_ptr<kernel::Kernel>> running_nodes;

    /* START COORDINATOR */
    auto coordinator = CreateCoordinator(settings, scene);

    for (int i = 0; i < node_count; i++) {

      running_kernels.push_back(std::move(new_kernel));
    }
  }
}