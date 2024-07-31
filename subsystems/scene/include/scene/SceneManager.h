#pragma once

#include "vsg/all.h"

namespace scene {

/**
 * Holds a scene and offers util functionality.
 */
class SceneManager {
protected:
  vsg::ref_ptr<vsg::StateGroup> m_scene;

public:
  SceneManager();
  virtual ~SceneManager() = default;

  vsg::ref_ptr<vsg::StateGroup> GetRoot() const noexcept;
};

inline vsg::ref_ptr<vsg::StateGroup> SceneManager::GetRoot() const noexcept {
  return m_scene;
}

typedef std::shared_ptr<SceneManager> SharedScene;

} // namespace scene
