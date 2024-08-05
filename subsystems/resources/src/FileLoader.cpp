#include "resources/loader/impl/FileLoader.h"
#include <fstream>

using namespace resources;
using namespace resources::loaders;

SharedResource FileLoader::Load(const std::string &uri) {

  std::ifstream ifs(uri, std::ios::binary | std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  auto bytes_in_file = std::make_shared<std::vector<char>>();
  if (pos != 0) {
    ifs.seekg(0, std::ios::beg);
    ifs.read(&bytes_in_file->operator[](0), pos);
  }

  SharedResource resource = std::make_shared<Resource>(bytes_in_file);

  return resource;
}