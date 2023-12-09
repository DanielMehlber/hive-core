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
  bool enable_rendering_job;
  int service_port;
  int width, height;
  int max_update_rate;

  po::options_description options("Allowed options");
  options.add_options()("help", "Shows this help message")(
      "renderer,r",
      po::value<std::string>(&renderer_type)->default_value("none"),
      "Type of renderer to use at startup (offscreen, onscreen, none are "
      "provided by default)")(
      "plugins,p", po::value<std::string>(&plugin_path)->default_value(""),
      "Path to directory that contains plugins")(
      "enable-render-service,rs", po::bool_switch(&enable_rendering_service),
      "Registers a rendering service on this instance")(
      "port", po::value<int>(&service_port)->default_value(-1),
      "Sets port to access network services of this instance and enables "
      "remote services")("enable-render-job",
                         po::bool_switch(&enable_rendering_job),
                         "Enables rendering job and continuous rendering")(
      "width,w", po::value<int>(&width)->default_value(500),
      "Initial width of the renderer")(
      "height,h", po::value<int>(&height)->default_value(500),
      "Initial height of the renderer")(
      "update,u", po::value<int>(&max_update_rate)->default_value(-1),
      "Rate limit of update operations");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << options << std::endl;
    return 1;
  }

  auto config = std::make_shared<common::config::Configuration>();
  bool local_only = service_port < 0;
  kernel::Kernel kernel(config, local_only);

  auto vsg_options = vsg::Options::create();
  vsg_options->sharedObjects = vsg::SharedObjects::create();
  vsg_options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
  vsg_options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

  auto scene = std::make_shared<scene::SceneManager>();
  auto node = vsg::read_cast<vsg::Node>("models/teapot.vsgt", vsg_options);
  auto skybox = vsg::read_cast<vsg::Node>("models/skybox.vsgt", vsg_options);

  if (!node || !skybox) {
    LOG_WARN("Demo scene not created: models not found")
  } else {
    scene->GetRoot()->addChild(node);
    scene->GetRoot()->addChild(skybox);

    auto light = vsg::DirectionalLight::create();
    light->color = {1, 1, 1};
    light->intensity = 20;
    light->direction = {-3, -3, -3};
    scene->GetRoot()->addChild(light);
  }

  if (renderer_type == "onscreen") {
    auto renderer =
        std::make_shared<graphics::OnscreenRenderer>(scene, width, height);
    kernel.SetRenderer(renderer);

    if (enable_rendering_job) {
      kernel.EnableRenderingJob();
    }

    if (enable_rendering_service) {
      auto offscreen_renderer = std::make_shared<graphics::OffscreenRenderer>(
          renderer->GetSetup(), scene);

      kernel.EnableRenderingService(offscreen_renderer);
      offscreen_renderer->Resize(width, height);
    }
  } else if (renderer_type == "offscreen") {
    graphics::RendererSetup info{};
    auto renderer = std::make_shared<graphics::OffscreenRenderer>(info, scene);
    kernel.SetRenderer(renderer);

    renderer->Resize(width, height);

    if (enable_rendering_job) {
      kernel.EnableRenderingJob();
    }

    if (enable_rendering_service) {
      kernel.EnableRenderingService();
    }
  }

  if (!plugin_path.empty()) {
    kernel.GetPluginManager()->InstallPlugins(plugin_path);
  }

  auto targetInterval = std::chrono::duration_cast<std::chrono::nanoseconds>(
      1000ms / max_update_rate);

  auto lastIterationTime = std::chrono::steady_clock::now();

  while (!kernel.ShouldShutdown()) {

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        currentTime - lastIterationTime);
    if (elapsed < targetInterval) {
      // Sleep to maintain the desired rate limit
      std::this_thread::sleep_for(targetInterval - elapsed);
    }
    lastIterationTime = std::chrono::steady_clock::now();
    kernel.GetJobManager()->InvokeCycleAndWait();
  }

  return 0;
}