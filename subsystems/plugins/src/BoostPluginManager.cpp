#include "plugins/impl/BoostPluginManager.h"
#include "jobsystem/manager/JobManager.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/json.hpp>
#include <list>
#include <map>

using namespace hive::plugins;
using namespace hive::jobsystem;

namespace json = boost::json;
namespace fs = boost::filesystem;

struct BoostPluginManager::Impl {
  /** List of all currently installed plugins */
  std::map<std::string, boost::shared_ptr<IPlugin>> plugins;
  mutable mutex plugins_mutex;
};

BoostPluginManager::BoostPluginManager(
    SharedPluginContext context,
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems)
    : m_impl(std::make_unique<Impl>()), m_context(std::move(context)),
      m_subsystems(subsystems) {}

void BoostPluginManager::LoadAndInstallPluginAsJob(const std::string &path) {
  auto absolute_path = fs::canonical(path).generic_string();

  boost::shared_ptr<IPlugin> plugin;
  plugin = boost::dll::import_symbol<IPlugin>(absolute_path, "plugin");

  if (plugin) {
    InstallPluginAsJob(plugin);
  } else {
    LOG_ERR("cannot load plugin: loaded library "
            << absolute_path << " does not provide a plugin symbol")
  }
} // namespace boost::filesystem

SharedPluginContext BoostPluginManager::GetContext() { return m_context; }

void BoostPluginManager::InstallPluginAsJob(boost::shared_ptr<IPlugin> plugin) {
  auto plugin_name = plugin->GetName();
  auto install_job = std::make_shared<Job>(
      [plugin_manager_ref = ReferenceFromThis(),
       plugin = std::move(plugin)](jobsystem::JobContext *context) mutable {
        if (auto maybe_plugin_manager = plugin_manager_ref.TryBorrow()) {
          auto plugin_manager = maybe_plugin_manager.value();
          try {
            plugin->Init(plugin_manager->GetContext());
          } catch (const std::exception &exception) {
            LOG_ERR("installation of plugin '"
                    << plugin->GetName()
                    << "' failed due to an exception in the plugin's "
                       "initialization routine: "
                    << exception.what())
            return JobContinuation::DISPOSE;
          }

          auto name = plugin->GetName();

          // add plugin to managed plugins
          std::unique_lock lock(plugin_manager->m_impl->plugins_mutex);
          plugin_manager->m_impl->plugins[name] = std::move(plugin);
          lock.unlock();

          LOG_INFO("loaded and installed plugin '" << name << "' successfully")
          return JobContinuation::DISPOSE;
        } else {
          LOG_ERR("cannot install plugin '"
                  << plugin->GetName()
                  << "' because plugin manager has already shut down")
          return JobContinuation::DISPOSE;
        }
      },
      "install-plugin-{" + plugin_name + "}", JobExecutionPhase::INIT);

  GetContext()
      ->GetSubsystems()
      .Borrow()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(install_job);
}

void BoostPluginManager::UnloadPlugin(const std::string &name) {
  std::unique_lock lock(m_impl->plugins_mutex);

  bool plugin_installed = m_impl->plugins.contains(name);
  if (!plugin_installed) {
    LOG_WARN("no plugin named '" << name << "' is currently loaded")
    return;
  }

  auto plugin = std::move(m_impl->plugins.at(name));

  try {
    plugin->ShutDown(GetContext());
  } catch (const std::exception &exception) {
    LOG_ERR("shut down routine of plugin '"
            << plugin->GetName()
            << "' threw an exception: " << exception.what())
    return;
  }

  // remove plugin from managed plugins
  m_impl->plugins.erase(name);
  plugin.reset();

  LOG_INFO("shut-down and unloaded plugin '" << name << "' successfully")
}

void BoostPluginManager::UnloadPluginAsJob(const std::string &name) {
  auto uninstall_job = std::make_shared<Job>(
      [plugin_manager_ref = ReferenceFromThis(),
       name](jobsystem::JobContext *context) mutable {
        if (auto maybe_plugin_manager = plugin_manager_ref.TryBorrow()) {
          auto plugin_manager = maybe_plugin_manager.value();
          plugin_manager->UnloadPlugin(name);
        } else {
          LOG_ERR("cannot uninstall plugin '"
                  << name << "' because plugin manager has already shut down")
        }

        return JobContinuation::DISPOSE;
      },
      "uninstall-plugin-{" + name + "}", JobExecutionPhase::CLEAN_UP);

  GetContext()
      ->GetSubsystems()
      .Borrow()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(uninstall_job);
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
      LOG_ERR("plugin descriptor has invalid JSON format. No plugins loaded.")
    }
  } catch (const std::exception &parsing_failure) {
    LOG_ERR("cannot parse plugin descriptor: " << parsing_failure.what())
  }

  return list;
}

void BoostPluginManager::LoadPluginsAsJob(const std::string &input_path_str) {

  // spawn job to load plugins in directory
  auto install_job = std::make_shared<Job>(
      [plugin_manager_ref = ReferenceFromThis(),
       input_path_str](jobsystem::JobContext *context) mutable {
        if (auto maybe_plugin_manager = plugin_manager_ref.TryBorrow()) {
          auto plugin_manager = maybe_plugin_manager.value();

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
            LOG_ERR("cannot install plugins from "
                    << input_path_str
                    << " because it is not a valid path: " << path_error.what())
            return DISPOSE;
          }

          bool is_directory = fs::is_directory(input_path_str);
          if (!is_directory) {
            LOG_WARN("cannot load plugins from "
                     << absolute_path << " because it is not a directory")
            return DISPOSE;
          }

          std::string jsonFilePath = absolute_path + "/plugins.json";
          std::vector<std::string> plugin_paths;
          bool plugin_descriptor_exists = fs::exists(jsonFilePath);
          if (plugin_descriptor_exists) {
            LOG_DEBUG("plugin descriptor found at " << jsonFilePath)

            // load descriptor content using loader
            auto resource_manager =
                plugin_manager->m_subsystems.Borrow()
                    ->RequireSubsystem<resources::IResourceManager>();
            auto future_resource =
                resource_manager->LoadResource("file://" + jsonFilePath);
            context->GetJobManager()->Await(future_resource);

            std::shared_ptr<std::vector<char>> content;
            try {
              content =
                  future_resource.get()->ExtractAsType<std::vector<char>>();
            } catch (const std::exception &exception) {
              LOG_ERR("cannot load contents of plugin descriptor at "
                      << jsonFilePath << ": " << exception.what())
              return JobContinuation::DISPOSE;
            }

            std::string descriptor(content->begin(), content->end());
            plugin_paths = std::move(listAllPluginsInDescriptor(descriptor));

          } else {
            LOG_WARN("no plugin descriptor found in '"
                     << absolute_path
                     << "': attempting to load all shared libraries; please "
                        "add a plugin descriptor")

            plugin_paths =
                std::move(listAllSharedLibsInDirectory(input_path_str));
            if (plugin_paths.empty()) {
              LOG_WARN("no plugin loaded because there were no shared "
                       "libraries in the given directory ("
                       << absolute_path << ")")
              return JobContinuation::DISPOSE;
            }

            LOG_DEBUG("found "
                      << plugin_paths.size()
                      << " possible shared libs that can be loaded as plugins")
          }

          LOG_INFO("attempting to install " << plugin_paths.size() << " from "
                                            << absolute_path)

          for (const auto &plugin_path : plugin_paths) {
            plugin_manager->LoadAndInstallPluginAsJob(plugin_path);
          }

          return JobContinuation::DISPOSE;

        } else {
          LOG_ERR("cannot install plugins from "
                  << input_path_str << " because plugin manager has shut down")
          return JobContinuation::DISPOSE;
        }
      },
      "install-plugins-in-{" + input_path_str + "}", JobExecutionPhase::INIT);

  GetContext()
      ->GetSubsystems()
      .Borrow()
      ->GetSubsystem<jobsystem::JobManager>()
      .value()
      ->KickJob(install_job);
}

BoostPluginManager::~BoostPluginManager() {
  std::list<std::string> current_plugins;

  {
    std::unique_lock lock(m_impl->plugins_mutex);
    for (const auto &plugin : m_impl->plugins) {
      current_plugins.push_back(plugin.first);
    }
  }

  for (const auto &plugin_name : current_plugins) {
    UnloadPlugin(plugin_name);
  }

  LOG_DEBUG("shared library plugin manager has been shut down")
}
