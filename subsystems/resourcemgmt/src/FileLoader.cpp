#include "resourcemgmt/loader/impl/FileLoader.h"
#include "resourcemgmt/ResourceFactory.h"
#include <fstream>

using namespace resourcemgmt;
using namespace resourcemgmt::loaders;

inline SharedResource FileLoader::Load(const std::string &uri) {

  std::ifstream ifs(uri, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  auto bytes_in_file = std::make_shared<std::vector<char>>();
  if (pos != 0) {
    ifs.seekg(0, std::ios::beg);
    ifs.read(&bytes_in_file->operator[](0), pos);
  }

  SharedResource resource =
      ResourceFactory::CreateSharedResource(bytes_in_file);

  return resource;
}