#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/variables_map.hpp"
#include "core/Core.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/renderer/impl/OnscreenRenderer.h"

using namespace hive;
namespace po = boost::program_options;

int main(int argc, const char **argv) {
  /* PARSE COMMAND LINE OPTIONS */
  std::string renderer_type;
  std::string plugin_path;
  bool enable_rendering_service;
  bool enable_rendering_job;
  int service_port;
  int width, height;
  int max_update_rate;
  unsigned int threads;
  std::vector<std::string> connections_to_establish;
  std::vector<std::string> scene_objects_to_load;

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
      "Rate limit of update operations")(
      "threads,t",
      po::value<unsigned int>(&threads)->default_value(
          std::thread::hardware_concurrency()),
      "amount of threads used in the job system")(
      "connect,c",
      po::value<std::vector<std::string>>(&connections_to_establish)
          ->multitoken(),
      "connection(s) to establish given in the format <host>:<port>")(
      "scene,s",
      po::value<std::vector<std::string>>(&scene_objects_to_load)->multitoken(),
      "Object(s) to load into the scene");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << options << std::endl;
    return 1;
  }

  /* APPLY OPTIONS TO SUBSYSTEM CONFIGURATION */
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("net.port", service_port);
  config->Set("jobs.concurrency", threads);
  bool local_only = service_port < 0;

  /* START KERNEL AND ALL ITS SUBSYSTEMS */
  core::Core core(config, local_only);

  /* LOAD SCENE FILES (IF ANY WERE SPECIFIED) */
  auto vsg_options = vsg::Options::create();
  vsg_options->sharedObjects = vsg::SharedObjects::create();
  vsg_options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
  vsg_options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

  auto scene = std::make_shared<scene::SceneManager>();

  bool objects_loaded{false};
  for (const auto &scene_object_file_path : scene_objects_to_load) {
    auto node = vsg::read_cast<vsg::Node>(scene_object_file_path, vsg_options);
    if (node) {
      scene->GetRoot()->addChild(node);
      objects_loaded = true;
    } else {
      LOG_ERR("Cannot load scene " << scene_object_file_path)
      return -1;
    }
  }

  if (!objects_loaded) {
    LOG_WARN("The scene is empty")
  } else {
    auto light = vsg::DirectionalLight::create();
    light->color = {1, 1, 1};
    light->intensity = 20;
    light->direction = {-3, -3, -3};
    scene->GetRoot()->addChild(light);
  }

  /* SETUP REQUESTED RENDERER AND ADD IT AS SUBSYSTEM TO KERNEL */
  if (renderer_type == "onscreen") {
    auto renderer =
        common::memory::Owner<graphics::OnscreenRenderer>(scene, width, height);
    auto renderer_ref = renderer.CreateReference();
    core.SetRenderer(std::move(renderer));

    if (enable_rendering_job) {
      core.EnableRenderingJob();
    }

    if (enable_rendering_service) {
      auto offscreen_renderer =
          common::memory::Owner<graphics::OffscreenRenderer>(
              renderer_ref.Borrow()->GetSetup(), scene);

      core.EnableRenderingService(offscreen_renderer.CreateReference());
      offscreen_renderer->Resize(width, height);

      /* We need to transfer the ownership of offscreen renderer somewhere. Use
       * the subsystem manager and instead of IRenderer (used by the main
       * renderer), use OffscreenRenderer type (for the service renderer). As a
       * result, the subsystem manager owns 2 renderers. */
      core.GetSubsystemsManager()
          ->AddOrReplaceSubsystem<graphics::OffscreenRenderer>(
              std::move(offscreen_renderer));
    }
  } else if (renderer_type == "offscreen") {
    auto renderer =
        common::memory::Owner<graphics::OffscreenRenderer>(std::nullopt, scene);

    renderer->Resize(width, height);

    core.SetRenderer(std::move(renderer));

    if (enable_rendering_job) {
      core.EnableRenderingJob();
    }

    if (enable_rendering_service) {
      core.EnableRenderingService();
    }
  }

  /* LOAD PLUGINS (IF ANY WERE SPECIFIED) */
  if (!plugin_path.empty()) {
    core.GetPluginManager()->InstallPlugins(plugin_path);
  }

  /* CONNECT TO OTHER PEERS (IF ANY WERE SPECIFIED AND SUBSYSTEM IS PROVIDED) */
  if (core.GetSubsystemsManager()
          ->ProvidesSubsystem<networking::messaging::IMessageEndpoint>()) {
    auto peer_networking_subsystem =
        core.GetSubsystemsManager()
            ->RequireSubsystem<networking::messaging::IMessageEndpoint>();
    for (const auto &connection_url : connections_to_establish) {
      peer_networking_subsystem->EstablishConnectionTo(connection_url);
    }
  }

  auto targetInterval = std::chrono::duration_cast<std::chrono::nanoseconds>(
      1000ms / max_update_rate);
  auto lastIterationTime = std::chrono::steady_clock::now();

  /* MAIN PROCESSING LOOP */
  while (!core.ShouldShutdown()) {

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        currentTime - lastIterationTime);
    if (elapsed < targetInterval) {
      // Sleep to maintain the desired rate limit
      std::this_thread::sleep_for(targetInterval - elapsed);
    }
    lastIterationTime = std::chrono::steady_clock::now();
    core.GetJobManager()->InvokeCycleAndWait();
  }

  return 0;
}