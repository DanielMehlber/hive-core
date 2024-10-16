#include "DemoPlugin.h"

using namespace std::chrono_literals;

std::string DemoPlugin::GetName() { return "demo-plugin"; }

void DemoPlugin::Init(hive::plugins::SharedPluginContext context) {
  LOG_INFO("Hello world from the demo plugin")
  LOG_DEBUG("This log will be hidden in release mode")

  // Access the subsystems of the running core
  auto subsystems = context->GetSubsystems().Borrow();

  // Create a job that says hello every 5 seconds
  auto hello_job = std::make_shared<hive::jobsystem::TimerJob>(
      [](hive::jobsystem::JobContext *context) {
        LOG_INFO("Hello from the demo plugin")
        return hive::jobsystem::JobContinuation::REQUEUE;
      },
      "demo-hello", 5s);

  // Submit that job to the core for execution
  auto job_system = subsystems->RequireSubsystem<hive::jobsystem::JobManager>();
  job_system->KickJob(hello_job);
}

void DemoPlugin::ShutDown(hive::plugins::SharedPluginContext context) {
  LOG_INFO("Goodbye from the demo plugin")

  auto subsystems = context->GetSubsystems().Borrow();
  auto job_system = subsystems->RequireSubsystem<hive::jobsystem::JobManager>();

  // Cancel the job that says hello every 5 seconds
  job_system->DetachJob("demo-hello");
}

// The following code is required to export these symbols for the plugin loader
extern "C" BOOST_SYMBOL_EXPORT DemoPlugin plugin;
DemoPlugin plugin;