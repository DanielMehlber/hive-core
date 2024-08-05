#include "scene/SceneManager.h"

using namespace hive::scene;

SceneManager::SceneManager() { m_scene = vsg::StateGroup::create(); }