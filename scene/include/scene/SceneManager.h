#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Scene.h"
#include "vsg/all.h"

namespace scene {

/**
 * Holds a scene and offers util functionality.
 */
class SCENE_API SceneManager {
protected:
  vsg::ref_ptr<vsg::Group> m_scene;

public:
  SceneManager();
  virtual ~SceneManager() = default;

  vsg::ref_ptr<vsg::Group> GetRoot() const noexcept;
};

inline vsg::ref_ptr<vsg::Group> SceneManager::GetRoot() const noexcept {
  return m_scene;
}

typedef std::shared_ptr<SceneManager> SharedScene;

} // namespace scene

#endif // SCENEMANAGER_H
