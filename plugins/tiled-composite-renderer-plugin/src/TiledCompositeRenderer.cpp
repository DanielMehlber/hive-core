#include "TiledCompositeRenderer.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "jobsystem/manager/JobManager.h"
#include "services/registry/IServiceRegistry.h"

using namespace std::chrono_literals;

TiledCompositeRenderer::TiledCompositeRenderer(
    const common::subsystems::SharedSubsystemManager &subsystems,
    graphics::SharedRenderer output_renderer)
    : m_subsystems(subsystems), m_output_renderer(std::move(output_renderer)) {
  m_camera = vsg::Camera::create();

#ifndef NDEBUG
  m_frames_per_second = std::make_shared<std::atomic<int>>(0);
  auto job = std::make_shared<TimerJob>(
      [frames_per_second = std::weak_ptr<std::atomic<int>>(
           m_frames_per_second)](jobsystem::JobContext *context) {
        if (frames_per_second.expired()) {
          return JobContinuation::DISPOSE;
        }

        auto fps = frames_per_second.lock();
        int _fps = fps->exchange(0);

        LOG_DEBUG("Current Tiled Composite Renderer FPS: " << _fps)

        return JobContinuation::REQUEUE;
      },
      "render-debug-fps", 1s, CLEAN_UP);

  auto job_system =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
  job_system->KickJob(job);

#endif
}

std::optional<graphics::SharedRenderResult>
TiledCompositeRenderer::GetResult() {
  return m_output_renderer->GetResult();
}

void TiledCompositeRenderer::SetScene(const scene::SharedScene &scene){
    LOG_WARN("Cannot set scene using the TiledCompositeRenderer")}

scene::SharedScene TiledCompositeRenderer::GetScene() const {
  LOG_WARN("Cannot get scene using the TiledCompositeRenderer")
  return nullptr;
}

void TiledCompositeRenderer::Resize(int width, int height) {
  m_output_renderer->Resize(width, height);
}

std::tuple<int, int> TiledCompositeRenderer::GetCurrentSize() const {
  return m_output_renderer->GetCurrentSize();
}

void TiledCompositeRenderer::SetCameraProjectionMatrix(
    vsg::ref_ptr<vsg::ProjectionMatrix> matrix) {
  m_camera->projectionMatrix = matrix;
}

void TiledCompositeRenderer::SetCameraViewMatrix(
    vsg::ref_ptr<vsg::ViewMatrix> matrix) {
  m_camera->viewMatrix = matrix;
}

bool TiledCompositeRenderer::Render() {
  auto job_system =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();

  auto service_registry =
      m_subsystems.lock()->RequireSubsystem<services::IServiceRegistry>();

  auto fut_caller = service_registry->Find("render");
  job_system->WaitForCompletion(fut_caller);

  auto opt_caller = fut_caller.get();
  if (!opt_caller.has_value()) {
    LOG_ERR("No render service available")
    return true;
  }

  auto service_caller = opt_caller->get();
  auto service_count = service_caller->GetCallableCount();
  if (service_count < 1) {
    LOG_ERR("No render service available")
    return true;
  }

  int segment_width = std::get<0>(GetCurrentSize()) / service_count;
  int segment_height = std::get<1>(GetCurrentSize());

  std::vector<vsg::ref_ptr<vsg::Image>> images;
  images.resize(service_count);

  auto counter = std::make_shared<jobsystem::job::JobCounter>();
  for (int i = 0; i < service_count; i++) {

    // build render service request
    graphics::RenderServiceRequest request;
    request.SetExtend({segment_width, segment_height});
    request.SetCameraData(graphics::CameraData{
        m_camera_info.up, m_camera_info.position, m_camera_info.direction});
    request.SetPerspectiveProjection(graphics::Perspective{0.01, 2000, 30});

    auto rendering_service_request = request.GetRequest();

    auto job = std::make_shared<jobsystem::job::Job>(
        [_this = weak_from_this(), rendering_service_request,
         subsystems = m_subsystems, service_caller, segment_height,
         segment_width, i, &images](jobsystem::JobContext *context) {
          auto fut_response = service_caller->Call(rendering_service_request,
                                                   context->GetJobManager());

          context->GetJobManager()->WaitForCompletion(fut_response);
          auto response = fut_response.get();

          if (response->GetStatus() == services::OK) {
            auto color_buffer_encoded =
                std::move(response->GetResult("color").value());

            auto encoding = response->GetResult("encoding").value();
            auto decoder =
                subsystems.lock()
                    ->RequireSubsystem<graphics::IRenderResultEncoder>();

            if (encoding == decoder->GetName()) {
              auto color_buffer =
                  std::move(decoder->Decode(color_buffer_encoded));

              // TODO: read format from image instead assuming it
              auto color_image = _this.lock()->LoadBufferIntoImage(
                  color_buffer, VK_FORMAT_R8G8B8A8_UNORM, segment_width,
                  segment_height);

              images[i] = color_image;
            } else {
              LOG_ERR("Cannot decode image from remote render result because "
                      "decoder for "
                      << encoding << " is not registered")
              return JobContinuation::DISPOSE;
            }
          }

          return JobContinuation::DISPOSE;
        });
    job->AddCounter(counter);
    job_system->KickJob(job);
  }

  job_system->WaitForCompletion(counter);

#ifndef NDEBUG
  m_frames_per_second->operator++();
#endif

  return true;
}

vsg::ref_ptr<vsg::Image> TiledCompositeRenderer::LoadBufferIntoImage(
    const std::vector<unsigned char> &buffer, VkFormat format, uint32_t width,
    uint32_t height) const {

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  VkDeviceSize imageSize = buffer.size();

  auto vsg_device = GetVulkanDevice();
  VkDevice device = vsg_device->vk();

  VkPhysicalDevice physicalDevice = vsg_device->getPhysicalDevice()->vk();
  uint32_t queueFamily = vsg_device->getQueues()[0]->queueFamilyIndex();

  auto vsg_queue = vsg_device->getQueue(queueFamily);
  VkQueue queue = vsg_queue->vk();

  auto vsg_command_pool = vsg::CommandPool::create(vsg_device, queueFamily);
  VkCommandPool pool = vsg_command_pool->vk();

  utils::createBuffer(device, physicalDevice, imageSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      staging_buffer, staging_buffer_memory);

  void *data;
  vkMapMemory(device, staging_buffer_memory, 0, imageSize, 0, &data);
  memcpy(data, buffer.data(), static_cast<size_t>(imageSize));
  vkUnmapMemory(device, staging_buffer_memory);

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  //  utils::createImage(textureImage, textureImageMemory, width, height,
  //  format,
  //                     device, physicalDevice);

  utils::transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device,
                               pool, queue);

  utils::copyBufferToImage(staging_buffer, textureImage, pool, device, queue,
                           width, height);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);

  // TODO: attach device memory somehow to avoid memory leak
  auto vsg_image = vsg::Image::create(textureImage, vsg_device);
  vsg_image->extent = {width, height};
  vsg_image->format = format;

  return vsg_image;
}

vsg::ref_ptr<vsg::Device> TiledCompositeRenderer::GetVulkanDevice() const {
  return m_output_renderer->GetVulkanDevice();
}
