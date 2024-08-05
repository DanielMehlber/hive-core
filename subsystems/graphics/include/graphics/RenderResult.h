#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace hive::graphics {

typedef std::vector<unsigned char> data_type;

/**
 * Allows access to the image generated in a rendering pass for further
 * processing.
 */
class RenderResult {
protected:
  /** width of image data in buffers */
  unsigned int m_width;
  /** height of image data in buffers */
  unsigned int m_height;
  /** buffer containing color data */
  data_type m_color_data;
  /** buffer containing depth data */
  data_type m_depth_data;
  /** Encoding of depth data in buffer */
  VkFormat m_depth_format;
  /** Encoding of color data in buffer */
  VkFormat m_color_format;

public:
  RenderResult() = default;
  unsigned int GetWidth() const;
  unsigned int GetHeight() const;

  void SetExtent(unsigned int width, unsigned int height);

  void SetColorData(data_type &&color_data, VkFormat format);
  void SetDepthData(data_type &&depth_data, VkFormat format);
  data_type &&ExtractColorData();
  data_type &&ExtractDepthData();
};

inline unsigned int RenderResult::GetWidth() const { return m_width; }

inline unsigned int RenderResult::GetHeight() const { return m_height; }

inline data_type &&RenderResult::ExtractColorData() {
  return std::move(m_color_data);
}

inline data_type &&RenderResult::ExtractDepthData() {
  return std::move(m_depth_data);
}

inline void RenderResult::SetColorData(data_type &&color_data,
                                       VkFormat format) {
  m_color_data = color_data;
  m_color_format = format;
}

inline void RenderResult::SetDepthData(graphics::data_type &&depth_data,
                                       VkFormat format) {
  m_depth_data = depth_data;
  m_depth_format = format;
}

inline void RenderResult::SetExtent(unsigned int width, unsigned int height) {
  m_width = width;
  m_height = height;
}

typedef std::shared_ptr<RenderResult> SharedRenderResult;

} // namespace hive::graphics
