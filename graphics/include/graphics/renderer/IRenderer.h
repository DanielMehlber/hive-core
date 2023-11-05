#ifndef IRENDERER_H
#define IRENDERER_H

#include "graphics/RenderResult.h"
#include "scene/SceneManager.h"
#include <optional>

namespace graphics {

/**
 * Generic interface for some renderer
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
};

} // namespace graphics

#endif // IRENDERER_H
