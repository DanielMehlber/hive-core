#ifndef VULKANOPERATIONS_H
#define VULKANOPERATIONS_H

#include "vsg/all.h"

namespace utils {

void createBuffer(VkDevice &device, VkPhysicalDevice &physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);

void copyBufferToImage(VkBuffer &buffer, VkImage &image, VkCommandPool &pool,
                       VkDevice &device, VkQueue &queue, uint32_t width,
                       uint32_t height);

void transitionImageLayout(VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout,
                           VkDevice &device, VkCommandPool &pool,
                           VkQueue &queue);

void createImage(VkImage &image, VkDeviceMemory &imageMemory, uint32_t width,
                 uint32_t height, VkFormat format, VkDevice &device,
                 VkPhysicalDevice &physicalDevice);

} // namespace utils

#endif // VULKANOPERATIONS_H
