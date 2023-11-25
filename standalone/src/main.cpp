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

  vsg::Builder builder;
  auto scene = std::make_shared<scene::SceneManager>();
  vsg::StateInfo info;
  info.lighting = false;
  info.two_sided = true;
  scene->GetRoot()->addChild(builder.createSphere({}, info));

  if (renderer_type == "onscreen") {
    auto renderer = std::make_shared<graphics::OnscreenRenderer>();
    kernel.SetRenderer(renderer);
    kernel.EnableRenderingJob();

    if (enable_rendering_service) {
      auto offscreen_renderer = std::make_shared<graphics::OffscreenRenderer>(
          renderer->GetInfo(), scene);

      offscreen_renderer->Resize(100, 100);
      kernel.EnableRenderingService(offscreen_renderer);
    }
  } else if (renderer_type == "offscreen") {
    auto renderer = std::make_shared<graphics::OffscreenRenderer>();
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