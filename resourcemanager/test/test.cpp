#include <chrono>
#include <gtest/gtest.h>
#include <resourcemgmt/ResourceFactory.h>
#include <resourcemgmt/ResourceManager.h>
#include <resourcemgmt/loader/IResourceLoader.h>

using namespace resourcemgmt;
using namespace std::chrono_literals;

class DummyLoader : public IResourceLoader {
private:
  std::chrono::duration<long double> m_delay;

public:
  DummyLoader(std::chrono::duration<long double> delay = 0.5s)
      : m_delay{delay} {};

  virtual const std::string &GetId() const noexcept {
    const static std::string id = "dummy";
    return id;
  };

  virtual SharedResource Load(const std::string &uri) {
    std::this_thread::sleep_for(m_delay);
    return ResourceFactory::CreateSharedResource<std::string>("dummy");
  };
};

TEST(ResourceMgmt, register_loader) {
  ResourceManager manager;
  auto dummy_loader = std::make_shared<DummyLoader>(0.5s);
  manager.RegisterLoader(dummy_loader);

  std::future<SharedResource> future = manager.LoadResource("dummy://hallo");
  future.wait();
  auto result = future.get()->ExtractAsType<std::string>();

  ASSERT_TRUE(result->compare("dummy") == 0);

  manager.UnregisterLoader(dummy_loader->GetId());
  ASSERT_THROW(std::future<SharedResource> future =
                   manager.LoadResource("dummy://hallo"),
               ResourceLoaderNotFound);
}

TEST(ResoureMgmt, loading_multiple) {
  ResourceManager manager(2);
  auto dummy_loader = std::make_shared<DummyLoader>(0.01s);
  manager.RegisterLoader(dummy_loader);

  std::vector<std::shared_future<SharedResource>> futures;
  futures.reserve(100);
  for (int i = 0; i < 100; i++) {
    std::future<SharedResource> future = manager.LoadResource("dummy://hallo");
    futures.push_back(future.share());
  }

  for (auto &future : futures) {
    future.wait();
    auto result = future.get()->ExtractAsType<std::string>();
    ASSERT_TRUE(result->compare("dummy") == 0);
  }
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}