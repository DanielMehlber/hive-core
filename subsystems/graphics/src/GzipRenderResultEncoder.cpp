#include "graphics/service/encoders/impl/GzipRenderResultEncoder.h"
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <sstream>
#include <vector>

using namespace graphics;

std::string
GzipRenderResultEncoder::Encode(const std::vector<unsigned char> &buffer) {
  std::stringstream compressed;
  std::stringstream original;

  // Assuming 'data' is a vector<unsigned char>
  original.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());

  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
  out.push(boost::iostreams::gzip_compressor());
  out.push(compressed);
  boost::iostreams::copy(original, out);

  return compressed.str();
}

std::vector<unsigned char>
GzipRenderResultEncoder::Decode(const std::string &str) {
  std::vector<unsigned char> decompressed;

  std::stringstream compressed(str);

  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_decompressor());
  in.push(compressed);

  boost::iostreams::copy(in, std::back_inserter(decompressed));

  return decompressed;
}