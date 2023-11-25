#include "plugins/impl/BoostPluginManager.h"
#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/json.hpp>

using namespace plugins;
namespace json = boost::json;
namespace fs = boost::filesystem;

void BoostPluginManager::InstallPlugin(const std::string &path) {

  auto absolute_path = fs::canonical(path).generic_string();

  boost::shared_ptr<IPlugin> plugin;
  plugin = boost::dll::import_symbol<IPlugin>(absolute_path, "plugin");

  if (plugin) {
    InstallPlugin(plugin);
  } else {
    LOG_ERR("Cannot load plugin: loaded library "
            << absolute_path << " does not provide a plugin symbol");
    return;
  }
} // namespace boost::filesystem

SharedPluginContext BoostPluginManager::GetContext() { return m_context; }

void BoostPluginManager::InstallPlugin(boost::shared_ptr<IPlugin> plugin) {
  auto install_job = jobsystem::JobSystemFactory::CreateJob(
      [plugin_manager = weak_from_this(),
       plugin = std::move(plugin)](jobsystem::JobContext *context) {
        if (plugin_manager.expired()) {
          LOG_ERR("Cannot install plugin "
                  << plugin->GetName()
                  << " because Plugin Manager has already shut down")
          return JobContinuation::DISPOSE;
        }

        try {
          plugin->Init(plugin_manager.lock()->GetContext());
        } catch (const std::exception &exception) {
          LOG_ERR("Installation of plugin "
                  << plugin->GetName()
                  << " failed. Initialization threw exception: "
                  << exception.what())
          return JobContinuation::DISPOSE;
        }

        auto name = plugin->GetName();

        // add plugin to managed plugins
        std::unique_lock lock(plugin_manager.lock()->m_plugins_mutex);
        plugin_manager.lock()->m_plugins[name] = std::move(plugin);
        lock.unlock();

        LOG_INFO("Installed plugin " << name)
        return JobContinuation::DISPOSE;
      },
      "install-plugin-{" + plugin->GetName() + "}", JobExecutionPhase::INIT);

  GetContext()
      ->GetKernelSubsystems()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(install_job);
}

void BoostPluginManager::UninstallPlugin(const std::string &name) {
  auto install_job = jobsystem::JobSystemFactory::CreateJob(
      [plugin_manager = weak_from_this(),
       name](jobsystem::JobContext *context) {
        bool plugin_manager_destroyed = plugin_manager.expired();
        if (plugin_manager_destroyed) {
          LOG_ERR("Cannot uninstall plugin "
                  << name << " because Plugin Manager has already shut down")
          return JobContinuation::DISPOSE;
        }

        std::unique_lock lock(plugin_manager.lock()->m_plugins_mutex);

        bool plugin_installed = plugin_manager.lock()->m_plugins.contains(name);
        if (!plugin_installed) {
          LOG_WARN("Cannot uninstall non-existent Plugin " << name)
          return JobContinuation::DISPOSE;
        }

        auto plugin = std::move(plugin_manager.lock()->m_plugins.at(name));

        try {
          plugin->ShutDown(plugin_manager.lock()->GetContext());
        } catch (const std::exception &exception) {
          LOG_ERR("ShutDown of plugin "
                  << plugin->GetName()
                  << " failed due to throws exception: " << exception.what())
          return JobContinuation::DISPOSE;
        }

        // remove plugin from managed plugins
        plugin_manager.lock()->m_plugins.erase(name);
        plugin.reset();

        LOG_INFO("Uninstalled plugin " << name)

        return JobContinuation::DISPOSE;
      },
      "uninstall-plugin-{" + name + "}", JobExecutionPhase::CLEAN_UP);

  GetContext()
      ->GetKernelSubsystems()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(install_job);
}

std::vector<std::string> listAllSharedLibsInDirectory(const std::string &path) {
  std::vector<std::string> list;

  for (const auto &entry : fs::directory_iterator(path)) {
    if (fs::is_regular_file(entry.path()) &&
        entry.path().extension() == SHARED_LIB_EXTENSION) {
      list.push_back(entry.path().string());
    }
  }

  return list;
}

std::vector<std::string>
listAllPluginsInDescriptor(const std::string &descriptor) {
  std::vector<std::string> list;

  try {
    // Parse the JSON string
    json::value parsed_value = json::parse(descriptor);

    // Check if the parsed value is an object
    if (parsed_value.is_object()) {
      // Access the "plugins" array
      json::object json_object = parsed_value.get_object();
      json::array &plugins_array = json_object.at("plugins").as_array();

      // Loop through the elements in the "plugins" array
      for (const auto &element : plugins_array) {
        if (element.is_string()) {
          std::string plugin_name = element.as_string().c_str();
          list.push_back(plugin_name);
        }
      }
    } else {
      LOG_ERR("Plugin descriptor has invalid JSON format. No plugins found.")
    }
  } catch (const std::exception &parsing_failure) {
    LOG_ERR("Cannot parse plugin descriptor: " << parsing_failure.what())
  }

  return list;
}

void BoostPluginManager::InstallPlugins(const std::string &input_path_str) {

  // spawn job to load plugins in directory
  auto install_job = jobsystem::JobSystemFactory::CreateJob(
      [_this = weak_from_this(),
       input_path_str](jobsystem::JobContext *context) {
        if (_this.expired()) {
          LOG_ERR("Cannot install plugins from "
                  << input_path_str << " because plugin manager has shut down")
          return JobContinuation::DISPOSE;
        }

        fs::path input_path(input_path_str);

        std::string absolute_path;
        boost::system::error_code path_error;
        if (!input_path.is_absolute()) {
          absolute_path =
              fs::canonical(input_path_str, path_error).generic_string();
        } else {
          absolute_path = input_path.generic_string();
        }

        if (path_error.failed()) {
          LOG_ERR("Cannot install plugins from "
                  << input_path_str
                  << " because it is not a valid input_path_str: "
                  << path_error.what())
          return JobContinuation::DISPOSE;
        }

        bool is_directory = fs::is_directory(input_path_str);
        if (!is_directory) {
          LOG_WARN("Cannot load Plugins from "
                   << absolute_path << " because it is not a directory")
          return JobContinuation::DISPOSE;
        }

        std::string jsonFilePath = absolute_path + "/plugins.json";
        std::vector<std::string> plugin_paths;
        bool plugin_descriptor_exists = fs::exists(jsonFilePath);
        if (plugin_descriptor_exists) {
          LOG_DEBUG("Plugin descriptor found at " << jsonFilePath)

          // load descriptor content using loader
          auto resource_manager =
              _this.lock()
                  ->m_subsystems.lock()
                  ->RequireSubsystem<resourcemgmt::IResourceManager>();
          auto future_resource =
              resource_manager->LoadResource("file://" + jsonFilePath);
          context->GetJobManager()->WaitForCompletion(future_resource);

          std::shared_ptr<std::vector<char>> content;
          try {
            content = future_resource.get()->ExtractAsType<std::vector<char>>();
          } catch (const std::exception &exception) {
            LOG_ERR("Cannot load contents of plugin descriptor at "
                    << jsonFilePath << ": " << exception.what())
            return JobContinuation::DISPOSE;
          }

          std::string descriptor(content->begin(), content->end());
          plugin_paths = std::move(listAllPluginsInDescriptor(descriptor));

        } else {
          LOG_WARN("No Plugin descriptor found in '"
                   << absolute_path << "': Attempting to load all libraries")

          plugin_paths =
              std::move(listAllSharedLibsInDirectory(input_path_str));
          if (plugin_paths.empty()) {
            LOG_WARN("No Plugins loaded: No shared libraries in directory "
                     << absolute_path)
            return JobContinuation::DISPOSE;
          }

          LOG_DEBUG("Found "
                    << plugin_paths.size()
                    << " possible shared libs that can be loaded as Plugins")
        }

        LOG_INFO("Attempting to install " << plugin_paths.size() << " from "
                                          << absolute_path)

        for (const auto &plugin_path : plugin_paths) {
          _this.lock()->InstallPlugin(plugin_path);
        }

        return JobContinuation::DISPOSE;
      },
      "install-plugins-in-{" + input_path_str + "}", JobExecutionPhase::INIT);

  GetContext()
      ->GetKernelSubsystems()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(install_job);
}
