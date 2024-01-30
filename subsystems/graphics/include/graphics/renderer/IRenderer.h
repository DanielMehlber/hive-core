#ifndef IRENDERER_H
#define IRENDERER_H

#include "graphics/RenderResult.h"
#include "scene/SceneManager.h"
#include <optional>

namespace graphics {

/**
 * Contains Vulkan setup that can be used to create more renderers without
 * initializing them from the ground up.
 */
struct RendererSetup {
  vsg::ref_ptr<vsg::Device> device;
  vsg::ref_ptr<vsg::Instance> instance;
};

/**
 * Renderer which renders some scene with some camera setup and allows callers
 * to retrieve rendering results.
 * @note This is used to realize both on-screen and off-screen implementations
 * for rendering use cases.
 */
class IRenderer {
public:
  /**
   * Render current scene from current view
   */
  virtual bool Render() = 0;

  /**
   * @return Last RenderResult (if one exists)
   */
  virtual std::optional<SharedRenderResult> GetResult() = 0;

  virtual void SetScene(const scene::SharedScene &scene) = 0;
  virtual scene::SharedScene GetScene() const = 0;

  virtual void Resize(int width, int height) = 0;
  virtual std::tuple<int, int> GetCurrentSize() const = 0;

  virtual void
  SetCameraProjectionMatrix(vsg::ref_ptr<vsg::ProjectionMatrix> matrix) = 0;
  virtual void SetCameraViewMatrix(vsg::ref_ptr<vsg::ViewMatrix> matrix) = 0;

  /**
   * Retrieve this renderer's setup for creating another renderer.
   * @note To make multiple renderers use the same Vulkan device and instance,
   * this information can be extracted and used for other renderers.
   * @return current Vulkan setup (instance, device, ...)
   */
  virtual RendererSetup GetSetup() const = 0;
};

typedef std::shared_ptr<IRenderer> SharedRenderer;

} // namespace graphics

#endif // IRENDERER_H
