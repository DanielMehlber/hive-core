#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/renderer/impl/OnscreenRenderer.h"
#include "logging/LogManager.h"

int main() {

  auto scene = std::make_shared<scene::SceneManager>();

  auto object1 = vsg::read(
      "/home/danielmehlber/Downloads/VSG/vsgExamples/data/models/teapot.vsgt");
  auto node1 = object1.cast<vsg::Node>();
  if (node1) {
    LOG_INFO("loaded teapot");

    scene->GetRoot()->addChild(node1);
  }

  auto object2 = vsg::read("/home/danielmehlber/Downloads/VSG/vsgExamples/data/"
                           "models/lz.vsgt");
  auto node2 = object2.cast<vsg::Node>();
  if (node2) {
    LOG_INFO("loaded openstreetmap");

    scene->GetRoot()->addChild(node2);
  }

  auto renderer = graphics::OffscreenRenderer(scene);

  // renderer.SetScene(scene);

  renderer.Resize(100, 100);

  while (renderer.Render()) {
    auto result = renderer.GetResult().value();
    auto color_buffer = result->ExtractColorData();
    int count = 0;
    for (unsigned char c : color_buffer) {
      if (c != 0 && c != 255) {
        count++;
      }
    }

    LOG_INFO(count);
  }

  return 0;
}