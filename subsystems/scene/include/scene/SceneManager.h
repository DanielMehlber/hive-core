#pragma once

#include "vsg/all.h"

namespace hive::scene {

/**
 * Holds a scene and offers util functionality.
 */
class SceneManager {
protected:
  vsg::ref_ptr<vsg::StateGroup> m_scene;

public:
  SceneManager();
  virtual ~SceneManager() = default;

  vsg::ref_ptr<vsg::StateGroup> GetRoot() const ;
};

inline vsg::ref_ptr<vsg::StateGroup> SceneManager::GetRoot() const  {
  return m_scene;
}

typedef std::shared_ptr<SceneManager> SharedScene;

} // namespace hive::scene
