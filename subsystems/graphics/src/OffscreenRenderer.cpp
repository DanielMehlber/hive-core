#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "logging/LogManager.h"

using namespace graphics;

vsg::ref_ptr<vsg::ImageView>
createColorImageView(const vsg::ref_ptr<vsg::Device> &device,
                     const VkExtent2D &extent, VkFormat imageFormat,
                     VkSampleCountFlagBits samples) {
  auto colorImage = vsg::Image::create();
  colorImage->imageType = VK_IMAGE_TYPE_2D;
  colorImage->format = imageFormat;
  colorImage->extent = VkExtent3D{extent.width, extent.height, 1};
  colorImage->mipLevels = 1;
  colorImage->arrayLayers = 1;
  colorImage->samples = samples;
  colorImage->tiling = VK_IMAGE_TILING_OPTIMAL;
  colorImage->usage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  colorImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorImage->flags = 0;
  colorImage->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return vsg::createImageView(device, colorImage, VK_IMAGE_ASPECT_COLOR_BIT);
}

vsg::ref_ptr<vsg::ImageView>
createDepthImageView(const vsg::ref_ptr<vsg::Device> &device,
                     const VkExtent2D &extent, VkFormat depthFormat,
                     VkSampleCountFlagBits samples) {
  auto depthImage = vsg::Image::create();
  depthImage->imageType = VK_IMAGE_TYPE_2D;
  depthImage->extent = VkExtent3D{extent.width, extent.height, 1};
  depthImage->mipLevels = 1;
  depthImage->arrayLayers = 1;
  depthImage->samples = samples;
  depthImage->format = depthFormat;
  depthImage->tiling = VK_IMAGE_TILING_OPTIMAL;
  depthImage->usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  depthImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthImage->flags = 0;
  depthImage->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return vsg::createImageView(device, depthImage,
                              vsg::computeAspectFlagsForFormat(depthFormat));
}

std::pair<vsg::ref_ptr<vsg::Commands>, vsg::ref_ptr<vsg::Image>>
createColorCapture(const vsg::ref_ptr<vsg::Device> &device,
                   const VkExtent2D &extent,
                   const vsg::ref_ptr<vsg::Image> &sourceImage,
                   VkFormat sourceImageFormat) {
  auto width = extent.width;
  auto height = extent.height;

  auto physicalDevice = device->getPhysicalDevice();

  VkFormat targetImageFormat = sourceImageFormat;

  //
  // 1) Check to see if Blit is supported.
  //
  VkFormatProperties srcFormatProperties;
  vkGetPhysicalDeviceFormatProperties(*(physicalDevice), sourceImageFormat,
                                      &srcFormatProperties);

  VkFormatProperties destFormatProperties;
  vkGetPhysicalDeviceFormatProperties(
      *(physicalDevice), VK_FORMAT_R8G8B8A8_UNORM, &destFormatProperties);

  bool supportsBlit = ((srcFormatProperties.optimalTilingFeatures &
                        VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
                      ((destFormatProperties.linearTilingFeatures &
                        VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0);

  if (supportsBlit) {
    // we can automatically convert the image format when blit, so take
    // advantage of it to ensure RGBA
    targetImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
  }

  // std::cout<<"supportsBlit = "<<supportsBlit<<std::endl;

  //
  // 2) create image to write to
  //
  auto destinationImage = vsg::Image::create();
  destinationImage->imageType = VK_IMAGE_TYPE_2D;
  destinationImage->format = targetImageFormat;
  destinationImage->extent.width = width;
  destinationImage->extent.height = height;
  destinationImage->extent.depth = 1;
  destinationImage->arrayLayers = 1;
  destinationImage->mipLevels = 1;
  destinationImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  destinationImage->samples = VK_SAMPLE_COUNT_1_BIT;
  destinationImage->tiling = VK_IMAGE_TILING_LINEAR;
  destinationImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  destinationImage->compile(device);

  auto deviceMemory = vsg::DeviceMemory::create(
      device, destinationImage->getMemoryRequirements(device->deviceID),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  destinationImage->bind(deviceMemory, 0);

  //
  // 3) create command buffer and submit to graphics queue
  //
  auto commands = vsg::Commands::create();

  // 3.a) transition destinationImage to transfer destination initialLayout
  auto transitionDestinationImageToDestinationLayoutBarrier =
      vsg::ImageMemoryBarrier::create(
          0,                                    // srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT,         // dstAccessMask
          VK_IMAGE_LAYOUT_UNDEFINED,            // oldLayout
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
          VK_QUEUE_FAMILY_IGNORED,              // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,              // dstQueueFamilyIndex
          destinationImage,                     // image
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
          // subresourceRange
      );

  // 3.b) transition swapChainImage from present to transfer source
  // initialLayout
  auto transitionSourceImageToTransferSourceLayoutBarrier =
      vsg::ImageMemoryBarrier::create(
          VK_ACCESS_MEMORY_READ_BIT,            // srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,          // dstAccessMask
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,      // oldLayout
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // newLayout
          VK_QUEUE_FAMILY_IGNORED,              // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,              // dstQueueFamilyIndex
          sourceImage,                          // image
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
          // subresourceRange
      );

  auto cmd_transitionForTransferBarrier = vsg::PipelineBarrier::create(
      VK_PIPELINE_STAGE_TRANSFER_BIT,                       // srcStageMask
      VK_PIPELINE_STAGE_TRANSFER_BIT,                       // dstStageMask
      0,                                                    // dependencyFlags
      transitionDestinationImageToDestinationLayoutBarrier, // barrier
      transitionSourceImageToTransferSourceLayoutBarrier    // barrier
  );

  commands->addChild(cmd_transitionForTransferBarrier);

  if (supportsBlit) {
    // 3.c.1) if blit using vkCmdBlitImage
    VkImageBlit region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.srcOffsets[0] = VkOffset3D{0, 0, 0};
    region.srcOffsets[1] = VkOffset3D{static_cast<int32_t>(width),
                                      static_cast<int32_t>(height), 1};
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.dstOffsets[0] = VkOffset3D{0, 0, 0};
    region.dstOffsets[1] = VkOffset3D{static_cast<int32_t>(width),
                                      static_cast<int32_t>(height), 1};

    auto blitImage = vsg::BlitImage::create();
    blitImage->srcImage = sourceImage;
    blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitImage->dstImage = destinationImage;
    blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitImage->regions.push_back(region);
    blitImage->filter = VK_FILTER_NEAREST;

    commands->addChild(blitImage);
  } else {
    // 3.c.2) else use vkCmdCopyImage

    VkImageCopy region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    auto copyImage = vsg::CopyImage::create();
    copyImage->srcImage = sourceImage;
    copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    copyImage->dstImage = destinationImage;
    copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyImage->regions.push_back(region);

    commands->addChild(copyImage);
  }

  // 3.d) transition destination image from transfer destination layout to
  // general layout to enable mapping to image DeviceMemory
  auto transitionDestinationImageToMemoryReadBarrier =
      vsg::ImageMemoryBarrier::create(
          VK_ACCESS_TRANSFER_WRITE_BIT,         // srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,            // dstAccessMask
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // oldLayout
          VK_IMAGE_LAYOUT_GENERAL,              // newLayout
          VK_QUEUE_FAMILY_IGNORED,              // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,              // dstQueueFamilyIndex
          destinationImage,                     // image
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
          // subresourceRange
      );

  // 3.e) transition swap chain image back to present
  auto transitionSourceImageBackToPresentBarrier =
      vsg::ImageMemoryBarrier::create(
          VK_ACCESS_TRANSFER_READ_BIT,          // srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,            // dstAccessMask
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // oldLayout
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,      // newLayout
          VK_QUEUE_FAMILY_IGNORED,              // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,              // dstQueueFamilyIndex
          sourceImage,                          // image
          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
          // subresourceRange
      );

  auto cmd_transitionFromTransferBarrier = vsg::PipelineBarrier::create(
      VK_PIPELINE_STAGE_TRANSFER_BIT,                // srcStageMask
      VK_PIPELINE_STAGE_TRANSFER_BIT,                // dstStageMask
      0,                                             // dependencyFlags
      transitionDestinationImageToMemoryReadBarrier, // barrier
      transitionSourceImageBackToPresentBarrier      // barrier
  );

  commands->addChild(cmd_transitionFromTransferBarrier);

  return {commands, destinationImage};
}

std::pair<vsg::ref_ptr<vsg::Commands>, vsg::ref_ptr<vsg::Buffer>>
createDepthCapture(const vsg::ref_ptr<vsg::Device> &device,
                   const VkExtent2D &extent,
                   const vsg::ref_ptr<vsg::Image> &sourceImage,
                   VkFormat sourceImageFormat) {
  auto width = extent.width;
  auto height = extent.height;

  auto memoryRequirements =
      sourceImage->getMemoryRequirements(device->deviceID);

  // 1. create buffer to copy to.
  VkDeviceSize bufferSize = memoryRequirements.size;
  auto destinationBuffer = vsg::createBufferAndMemory(
      device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
          VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

  VkImageAspectFlags imageAspectFlags =
      vsg::computeAspectFlagsForFormat(sourceImageFormat);

  // 2.a) transition depth image for reading
  auto commands = vsg::Commands::create();

  auto transitionSourceImageToTransferSourceLayoutBarrier =
      vsg::ImageMemoryBarrier::create(
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // srcAccessMask
          VK_ACCESS_TRANSFER_READ_BIT,                      // dstAccessMask
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // oldLayout
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,             // newLayout
          VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
          sourceImage,             // image
          VkImageSubresourceRange{imageAspectFlags, 0, 1, 0, 1}
          // subresourceRange
      );

  auto transitionDestinationBufferToTransferWriteBarrier =
      vsg::BufferMemoryBarrier::create(
          VK_ACCESS_MEMORY_READ_BIT,    // srcAccessMask
          VK_ACCESS_TRANSFER_WRITE_BIT, // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,      // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,      // dstQueueFamilyIndex
          destinationBuffer,            // buffer
          0,                            // offset
          bufferSize                    // size
      );

  auto cmd_transitionSourceImageToTransferSourceLayoutBarrier =
      vsg::PipelineBarrier::create(
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
          VK_PIPELINE_STAGE_TRANSFER_BIT,                     // dstStageMask
          0,                                                  // dependencyFlags
          transitionSourceImageToTransferSourceLayoutBarrier, // barrier
          transitionDestinationBufferToTransferWriteBarrier   // barrier
      );
  commands->addChild(cmd_transitionSourceImageToTransferSourceLayoutBarrier);

  // 2.b) copy image to buffer
  {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength =
        width; // need to figure out actual row length from somewhere...
    region.bufferImageHeight = height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = VkOffset3D{0, 0, 0};
    region.imageExtent = VkExtent3D{width, height, 1};

    auto copyImage = vsg::CopyImageToBuffer::create();
    copyImage->srcImage = sourceImage;
    copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    copyImage->dstBuffer = destinationBuffer;
    copyImage->regions.push_back(region);

    commands->addChild(copyImage);
  }

  // 2.c) transition depth image back for rendering
  auto transitionSourceImageBackToPresentBarrier =
      vsg::ImageMemoryBarrier::create(
          VK_ACCESS_TRANSFER_READ_BIT, // srcAccessMask
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // dstAccessMask
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,             // oldLayout
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // newLayout
          VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
          sourceImage,             // image
          VkImageSubresourceRange{imageAspectFlags, 0, 1, 0, 1}
          // subresourceRange
      );

  auto transitionDestinationBufferToMemoryReadBarrier =
      vsg::BufferMemoryBarrier::create(
          VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,    // dstAccessMask
          VK_QUEUE_FAMILY_IGNORED,      // srcQueueFamilyIndex
          VK_QUEUE_FAMILY_IGNORED,      // dstQueueFamilyIndex
          destinationBuffer,            // buffer
          0,                            // offset
          bufferSize                    // size
      );

  auto cmd_transitionSourceImageBackToPresentBarrier =
      vsg::PipelineBarrier::create(
          VK_PIPELINE_STAGE_TRANSFER_BIT, // srcStageMask
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
          0,                                                 // dependencyFlags
          transitionSourceImageBackToPresentBarrier,         // barrier
          transitionDestinationBufferToMemoryReadBarrier     // barrier
      );

  commands->addChild(cmd_transitionSourceImageBackToPresentBarrier);

  return {commands, destinationBuffer};
}

void OffscreenRenderer::SetupInstanceAndDevice(
    const std::optional<RendererSetup> &existing_renderer_setup) {

  bool use_existing_renderer_setup = existing_renderer_setup.has_value();
  if (use_existing_renderer_setup) {
    LOG_DEBUG("using existing renderer setup for setting up offscreen renderer")
    // existing renderer setup contains pre-initialized device and instance
    const RendererSetup &renderer_setup = existing_renderer_setup.value();

    bool is_valid_setup = renderer_setup.device && renderer_setup.instance;
    if (is_valid_setup) {
      // use existing setup (possibly sharing it with another renderer)
      m_instance = renderer_setup.instance;
      m_device = renderer_setup.device;

      auto [physicalDevice, queueFamily] =
          m_instance->getPhysicalDeviceAndQueueFamily(VK_QUEUE_GRAPHICS_BIT);
      m_queueFamily = queueFamily;

      if (physicalDevice && queueFamily > 0) {
        // finish setup and do not continue creating own instance and device
        return;
      } else {
        LOG_ERR("cannot get physical Vulkan device from existing renderer "
                "setup. Trying to create a device instead...")
      }
    } else {
      LOG_WARN("existing renderer setup was invalid: either the device or "
               "instance is unusable. Trying to create them instead...")
    }
  }

  uint32_t vulkanVersion = VK_API_VERSION_1_2;

  vsg::Names instanceExtensions;
  vsg::Names requestedLayers;
#ifndef NBEGUB
  instanceExtensions.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  // requestedLayers.push_back("VK_LAYER_KHRONOS_validation");
  // requestedLayers.push_back("VK_LAYER_LUNARG_api_dump");
#endif

  vsg::Names validatedNames = vsg::validateInstancelayerNames(requestedLayers);

  m_instance =
      vsg::Instance::create(instanceExtensions, validatedNames, vulkanVersion);
  auto [physicalDevice, queueFamily] =
      m_instance->getPhysicalDeviceAndQueueFamily(VK_QUEUE_GRAPHICS_BIT);
  m_queueFamily = queueFamily;
  if (!physicalDevice || queueFamily < 0) {
    LOG_ERR("cannot create physical Vulkan device for offscreen renderer")
    THROW_EXCEPTION(
        RendererSetupException,
        "cannot create physical Vulkan device for offscreen renderer")
  }

  vsg::Names deviceExtensions;
  vsg::QueueSettings queueSettings{vsg::QueueSetting{queueFamily, {1.0}}};

  auto deviceFeatures = vsg::DeviceFeatures::create();
  deviceFeatures->get().samplerAnisotropy = VK_TRUE;
  deviceFeatures->get().geometryShader = true;

  m_device = vsg::Device::create(physicalDevice, queueSettings, validatedNames,
                                 deviceExtensions, deviceFeatures);
}

void OffscreenRenderer::SetupCamera() {

  if (!m_current_view_matrix) {
    // set up the camera
    m_current_view_matrix =
        vsg::LookAt::create(vsg::dvec3(0.0, 5, 0.0), vsg::dvec3(0, 0, 0),
                            vsg::dvec3(0.0, 0.0, 1.0));
  }

  if (!m_current_projection_matrix) {
    m_current_projection_matrix = vsg::Perspective::create(
        30.0,
        static_cast<double>(m_size.width) / static_cast<double>(m_size.height),
        0.001, 2000);
  }

  auto viewport_state = vsg::ViewportState::create(m_size);

  // if camera already exists, just modify it
  if (!m_camera) {
    m_camera = vsg::Camera::create(m_current_projection_matrix,
                                   m_current_view_matrix, viewport_state);
  } else {
    m_camera->projectionMatrix = m_current_projection_matrix;
    m_camera->viewMatrix = m_current_view_matrix;
    m_camera->viewportState = viewport_state;
  }
}

OffscreenRenderer::OffscreenRenderer(
    const std::optional<RendererSetup> &existing_renderer_setup,
    scene::SharedScene scene, bool use_depth_buffer, bool msaa,
    VkFormat imageFormat, VkFormat depthFormat,
    VkSampleCountFlagBits sample_count, VkExtent2D size)
    : m_color_image_format(imageFormat), m_depth_image_format(depthFormat),
      m_sample_count(sample_count), m_size(size), m_scene(std::move(scene)),
      m_use_depth_buffer(use_depth_buffer), m_msaa(msaa), m_queueFamily(-1) {

  if (m_msaa)
    m_sample_count = VK_SAMPLE_COUNT_8_BIT;

  SetupInstanceAndDevice(existing_renderer_setup);
  SetupCamera();

  m_scene_graph_root = vsg::StateGroup::create();
  m_scene_graph_root->addChild(m_scene->GetRoot());

  m_viewer = vsg::Viewer::create();
  SetupRenderGraph();
  SetupCommandGraph();

  /*
   * *** This is a workaround ***
   *
   * Somehow, when we initialize this renderer and directly resize it, the
   * rendered image is messed up: The rendered area is wrong.
   * It is only resolved by only resizing the renderer AFTER it once rendered.
   *
   * Might this be a bug in the VulkanSceneGraph RenderGraph? Its renderArea
   * attribute's value is messed up after direct resize. Waiting for
   * documentation of fix.
   *
   * -> Constructor -> render -> resize -> render -> ... [OKAY]
   * -> Constructor -> resize -> render -> ... [NOT OKAY]
   */
  Render();
}

void OffscreenRenderer::SetScene(const scene::SharedScene &scene) {
  m_scene_graph_root->children.clear();
  m_scene_graph_root->addChild(scene->GetRoot());
  m_scene = scene;
}

scene::SharedScene OffscreenRenderer::GetScene() const { return m_scene; }

bool OffscreenRenderer::Render() {
  if (m_viewer->advanceToNextFrame()) {
    // clear render result for next
    m_current_render_result.reset();

    m_viewer->handleEvents();
    m_viewer->update();
    m_viewer->recordAndSubmit();
    m_viewer->present();
    return true;
  } else {
    return false;
  }
}

void OffscreenRenderer::Resize(int width, int height) {

  m_viewer->deviceWaitIdle();

  width = std::max(width, 1);
  height = std::max(height, 1);

  VkExtent2D previous_size = m_size;

  m_size.width = width;
  m_size.height = height;

  // modify current camera
  m_camera->projectionMatrix->changeExtent(previous_size, m_size);
  m_camera->viewportState = vsg::ViewportState::create(m_size);

  // replace image views and capture commands (image size has changed)
  ReplaceFramebufferAndCaptures();

  LOG_INFO("offscreen renderer canvas was resized to " << width << "x"
                                                       << height)
}

std::optional<SharedRenderResult> OffscreenRenderer::GetResult() {
  uint64_t waitTimeout = 1999999999; // 1second in nanoseconds.

  SharedRenderResult render_result = std::make_shared<RenderResult>();

  if (m_copied_color_buffer || m_copied_depth_buffer) {
    // wait for completion.
    m_viewer->waitForFences(0, waitTimeout);

    std::vector<unsigned char> color_buffer;
    std::vector<unsigned char> depth_buffer;

    if (m_copied_color_buffer) {
      VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
      VkSubresourceLayout subResourceLayout;
      vkGetImageSubresourceLayout(*m_device,
                                  m_copied_color_buffer->vk(m_device->deviceID),
                                  &subResource, &subResourceLayout);

      auto deviceMemory =
          m_copied_color_buffer->getDeviceMemory(m_device->deviceID);

      size_t destRowWidth = m_size.width * sizeof(vsg::ubvec4);
      vsg::ref_ptr<vsg::Data> imageData;
      if (destRowWidth == subResourceLayout.rowPitch) {
        // Map the buffer memory and assign as a vec4Array2D that will
        // automatically unmap itself on destruction.
        imageData = vsg::MappedData<vsg::ubvec4Array2D>::create(
            deviceMemory, subResourceLayout.offset, 0,
            vsg::Data::Properties{m_color_image_format}, m_size.width,
            m_size.height);

      } else {
        // Map the buffer memory and assign as a ubyteArray that will
        // automatically unmap itself on destruction. A ubyteArray is used as
        // the graphics buffer memory is not contiguous like vsg::Array2D, so
        // map to a flat buffer first then copy to Array2D.
        auto mappedData = vsg::MappedData<vsg::ubyteArray>::create(
            deviceMemory, subResourceLayout.offset, 0,
            vsg::Data::Properties{m_color_image_format},
            subResourceLayout.rowPitch * m_size.height);
        imageData = vsg::ubvec4Array2D::create(
            m_size.width, m_size.height,
            vsg::Data::Properties{m_color_image_format});
        for (uint32_t row = 0; row < m_size.height; ++row) {
          std::memcpy(imageData->dataPointer(row * m_size.width),
                      mappedData->dataPointer(row * subResourceLayout.rowPitch),
                      destRowWidth);
        }
      }

      color_buffer.resize(imageData->dataSize());
      std::memcpy(color_buffer.data(), imageData->dataPointer(),
                  imageData->dataSize());

      render_result->SetExtent(imageData->width(), imageData->height());
    }

    if (m_copied_depth_buffer) {
      // 3. map buffer and copy data.
      auto deviceMemory =
          m_copied_depth_buffer->getDeviceMemory(m_device->deviceID);

      vsg::ref_ptr<vsg::Data> imageData;

      // Map the buffer memory and assign as a vec4Array2D that will
      // automatically unmap itself on destruction.
      if (m_depth_image_format == VK_FORMAT_D32_SFLOAT ||
          m_depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        imageData = vsg::MappedData<vsg::floatArray2D>::create(
            deviceMemory, 0, 0, vsg::Data::Properties{m_depth_image_format},
            m_size.width,
            m_size.height); // deviceMemory, offset, flags and dimensions

      } else {
        imageData = vsg::MappedData<vsg::uintArray2D>::create(
            deviceMemory, 0, 0, vsg::Data::Properties{m_depth_image_format},
            m_size.width,
            m_size.height); // deviceMemory, offset, flags and dimensions
      }

      depth_buffer.resize(imageData->dataSize());
      std::memcpy(depth_buffer.data(), imageData->dataPointer(),
                  imageData->dataSize());
    }

    render_result->SetDepthData(std::move(depth_buffer), m_depth_image_format);
    render_result->SetColorData(std::move(color_buffer), m_color_image_format);
  }

  return render_result;
}

void OffscreenRenderer::SetupRenderGraph() {

  vsg::ref_ptr<vsg::Framebuffer> framebuffer;

  if (m_use_depth_buffer) {
    m_color_image_view = createColorImageView(
        m_device, m_size, m_color_image_format, VK_SAMPLE_COUNT_1_BIT);
    m_depth_image_view = createDepthImageView(
        m_device, m_size, m_depth_image_format, VK_SAMPLE_COUNT_1_BIT);
    if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
      auto renderPass = vsg::createRenderPass(m_device, m_color_image_format,
                                              m_depth_image_format, true);
      framebuffer = vsg::Framebuffer::create(
          renderPass, vsg::ImageViews{m_color_image_view, m_depth_image_view},
          m_size.width, m_size.height, 1);
    } else {
      auto msaa_m_colorImageView = createColorImageView(
          m_device, m_size, m_color_image_format, m_sample_count);
      auto msaa_m_depthImageView = createDepthImageView(
          m_device, m_size, m_depth_image_format, m_sample_count);

      auto renderPass = vsg::createMultisampledRenderPass(
          m_device, m_color_image_format, m_depth_image_format, m_sample_count,
          true);
      framebuffer = vsg::Framebuffer::create(
          renderPass,
          vsg::ImageViews{msaa_m_colorImageView, m_color_image_view,
                          msaa_m_depthImageView, m_depth_image_view},
          m_size.width, m_size.height, 1);
    }

    // create support for copying the color buffer
    std::tie(m_color_buffer_capture, m_copied_color_buffer) =
        createColorCapture(m_device, m_size, m_color_image_view->image,
                           m_color_image_format);
    std::tie(m_depth_buffer_capture, m_copied_depth_buffer) =
        createDepthCapture(m_device, m_size, m_depth_image_view->image,
                           m_depth_image_format);
  } else {
    m_color_image_view = createColorImageView(
        m_device, m_size, m_color_image_format, VK_SAMPLE_COUNT_1_BIT);
    if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
      auto renderPass = vsg::createRenderPass(m_device, m_color_image_format);
      framebuffer = vsg::Framebuffer::create(
          renderPass, vsg::ImageViews{m_color_image_view}, m_size.width,
          m_size.height, 1);
    } else {
      auto msaa_m_colorImageView = createColorImageView(
          m_device, m_size, m_color_image_format, m_sample_count);

      auto renderPass = vsg::createMultisampledRenderPass(
          m_device, m_color_image_format, m_sample_count);
      framebuffer = vsg::Framebuffer::create(
          renderPass,
          vsg::ImageViews{msaa_m_colorImageView, m_color_image_view},
          m_size.width, m_size.height, 1);
    }

    // create support for copying the color buffer
    std::tie(m_color_buffer_capture, m_copied_color_buffer) =
        createColorCapture(m_device, m_size, m_color_image_view->image,
                           m_color_image_format);
  }

  m_render_graph = vsg::RenderGraph::create();

  m_render_graph->framebuffer = framebuffer;
  m_render_graph->renderArea.offset = {0, 0};
  m_render_graph->renderArea.extent = m_size;
  m_render_graph->setClearValues({{1.0f, 1.0f, 0.0f, 0.0f}},
                                 VkClearDepthStencilValue{0.0f, 0});
}

void OffscreenRenderer::SetupCommandGraph() {
  auto view = vsg::View::create(m_camera, m_scene_graph_root);

  vsg::CommandGraphs commandGraphs;
  m_render_graph->addChild(view);

  m_command_graph = vsg::CommandGraph::create(m_device, m_queueFamily);
  m_command_graph->addChild(m_render_graph);
  commandGraphs.push_back(m_command_graph);
  if (m_color_buffer_capture)
    m_command_graph->addChild(m_color_buffer_capture);
  if (m_depth_buffer_capture)
    m_command_graph->addChild(m_depth_buffer_capture);

  m_viewer->assignRecordAndSubmitTaskAndPresentation(commandGraphs);
  m_viewer->compile();
}

std::tuple<int, int> OffscreenRenderer::GetCurrentSize() const {
  return std::make_tuple(m_size.width, m_size.height);
}

void OffscreenRenderer::SetCameraProjectionMatrix(
    vsg::ref_ptr<vsg::ProjectionMatrix> matrix) {
  m_camera->projectionMatrix = matrix;
  m_current_projection_matrix = matrix;
}

void OffscreenRenderer::SetCameraViewMatrix(
    vsg::ref_ptr<vsg::ViewMatrix> matrix) {
  m_camera->viewMatrix = matrix;
  m_current_view_matrix = matrix;
}

void replaceChild(vsg::Group *group, const vsg::ref_ptr<vsg::Node> &previous,
                  const vsg::ref_ptr<vsg::Node> &replacement) {

  for (auto &child : group->children) {
    if (child == previous)
      child = replacement;
  }
}

void OffscreenRenderer::ReplaceFramebufferAndCaptures() {
  auto previous_colorBufferCapture = m_color_buffer_capture;
  auto previous_depthBufferCapture = m_depth_buffer_capture;

  vsg::ref_ptr<vsg::Framebuffer> framebuffer;

  if (m_use_depth_buffer) {
    m_color_image_view = createColorImageView(
        m_device, m_size, m_color_image_format, VK_SAMPLE_COUNT_1_BIT);
    m_depth_image_view = createDepthImageView(
        m_device, m_size, m_depth_image_format, VK_SAMPLE_COUNT_1_BIT);
    if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
      auto renderPass = vsg::createRenderPass(m_device, m_color_image_format,
                                              m_depth_image_format, true);
      framebuffer = vsg::Framebuffer::create(
          renderPass, vsg::ImageViews{m_color_image_view, m_depth_image_view},
          m_size.width, m_size.height, 1);
    } else {
      auto msaa_colorImageView = createColorImageView(
          m_device, m_size, m_color_image_format, m_sample_count);
      auto msaa_depthImageView = createDepthImageView(
          m_device, m_size, m_depth_image_format, m_sample_count);

      auto renderPass = vsg::createMultisampledRenderPass(
          m_device, m_color_image_format, m_depth_image_format, m_sample_count,
          true);
      framebuffer = vsg::Framebuffer::create(
          renderPass,
          vsg::ImageViews{msaa_colorImageView, m_color_image_view,
                          msaa_depthImageView, m_depth_image_view},
          m_size.width, m_size.height, 1);
    }

    m_render_graph->framebuffer = framebuffer;

    // create new copy subgraphs
    std::tie(m_color_buffer_capture, m_copied_color_buffer) =
        createColorCapture(m_device, m_size, m_color_image_view->image,
                           m_color_image_format);
    std::tie(m_depth_buffer_capture, m_copied_depth_buffer) =
        createDepthCapture(m_device, m_size, m_depth_image_view->image,
                           m_depth_image_format);

    replaceChild(m_command_graph, previous_colorBufferCapture,
                 m_color_buffer_capture);
    replaceChild(m_command_graph, previous_depthBufferCapture,
                 m_depth_buffer_capture);
  } else {
    m_color_image_view = createColorImageView(
        m_device, m_size, m_color_image_format, VK_SAMPLE_COUNT_1_BIT);
    if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
      auto renderPass = vsg::createRenderPass(m_device, m_color_image_format);
      framebuffer = vsg::Framebuffer::create(
          renderPass, vsg::ImageViews{m_color_image_view}, m_size.width,
          m_size.height, 1);
    } else {
      auto msaa_colorImageView = createColorImageView(
          m_device, m_size, m_color_image_format, m_sample_count);

      auto renderPass = vsg::createMultisampledRenderPass(
          m_device, m_color_image_format, m_sample_count);
      framebuffer = vsg::Framebuffer::create(
          renderPass, vsg::ImageViews{msaa_colorImageView, m_color_image_view},
          m_size.width, m_size.height, 1);
    }

    m_render_graph->framebuffer = framebuffer;

    // create new copy subgraphs
    std::tie(m_color_buffer_capture, m_copied_color_buffer) =
        createColorCapture(m_device, m_size, m_color_image_view->image,
                           m_color_image_format);

    replaceChild(m_command_graph, previous_colorBufferCapture,
                 m_color_buffer_capture);
    replaceChild(m_command_graph, previous_depthBufferCapture,
                 m_depth_buffer_capture);
  }
}
