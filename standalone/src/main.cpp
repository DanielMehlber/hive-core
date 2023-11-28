#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/variables_map.hpp"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/renderer/impl/OnscreenRenderer.h"
#include "kernel/Kernel.h"

namespace po = boost::program_options;

int main(int argc, const char **argv) {

  std::string renderer_type;
  std::string plugin_path;
  bool enable_rendering_service;

  po::options_description options("Allowed options");
  options.add_options()("help", "Shows this help message")(
      "renderer,r",
      po::value<std::string>(&renderer_type)->default_value("none"),
      "Type of renderer to use at startup (offscreen, onscreen, none are "
      "provided by default)")(
      "plugins,p", po::value<std::string>(&plugin_path)->default_value(""),
      "Path to directory that contains plugins")(
      "render-svc,rs",
      po::value<bool>(&enable_rendering_service)->default_value(false));

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << options << std::endl;
    return 1;
  }

  kernel::Kernel kernel;

  auto vsg_options = vsg::Options::create();
  vsg_options->sharedObjects = vsg::SharedObjects::create();
  vsg_options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
  vsg_options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

  auto scene = std::make_shared<scene::SceneManager>();

  auto light = vsg::DirectionalLight::create();
  light->color = {1, 1, 1};
  light->intensity = 20;
  light->direction = {-3, -3, -3};
  scene->GetRoot()->addChild(light);

  auto node = vsg::read_cast<vsg::Node>("models/teapot.vsgt", vsg_options);
  scene->GetRoot()->addChild(node);

  auto skybox = vsg::read_cast<vsg::Node>("models/skybox.vsgt", vsg_options);
  scene->GetRoot()->addChild(skybox);

  if (renderer_type == "onscreen") {
    auto renderer = std::make_shared<graphics::OnscreenRenderer>(scene);
    kernel.SetRenderer(renderer);
    kernel.EnableRenderingJob();

    if (enable_rendering_service) {
      auto offscreen_renderer = std::make_shared<graphics::OffscreenRenderer>(
          renderer->GetInfo(), scene);

      kernel.EnableRenderingService(offscreen_renderer);
      offscreen_renderer->Resize(500, 500);
    }
  } else if (renderer_type == "offscreen") {
    graphics::RendererInfo info{};
    auto renderer = std::make_shared<graphics::OffscreenRenderer>(info, scene);
    kernel.SetRenderer(renderer);
    kernel.EnableRenderingJob();

    renderer->Resize(500, 500);

    if (enable_rendering_service) {
      kernel.EnableRenderingService();
    }
  }

  if (!plugin_path.empty()) {
    kernel.GetPluginManager()->InstallPlugins(plugin_path);
  }

  while (true) {
    kernel.GetJobManager()->InvokeCycleAndWait();
  }

  return 0;
}