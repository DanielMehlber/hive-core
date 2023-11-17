#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace common::coding {

std::string base64_encode(const std::vector<unsigned char> &input) {
  using namespace boost::archive::iterators;

  typedef base64_from_binary<
      transform_width<std::vector<unsigned char>::const_iterator, 6, 8>>
      base64_enc;

  std::stringstream result;
  std::copy(base64_enc(input.begin()), base64_enc(input.end()),
            std::ostream_iterator<char>(result));
  return result.str();
}

std::vector<unsigned char> base64_decode(const std::string &input) {
  using namespace boost::archive::iterators;

  typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>
      base64_dec;

  std::stringstream result;
  std::copy(base64_dec(input.begin()), base64_dec(input.end()),
            std::ostream_iterator<char>(result));

  // Convert the decoded string to a vector of unsigned char
  std::string decoded_str = result.str();
  return std::vector<unsigned char>(decoded_str.begin(), decoded_str.end());
}
} // namespace common::coding