#include "utils/base64.h"

namespace radicc {
namespace {

constexpr char kBase64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

}  // namespace

std::string encode_base64(std::string_view input) {
  std::string output;
  output.reserve(((input.size() + 2) / 3) * 4);

  for (size_t i = 0; i < input.size(); i += 3) {
    const unsigned char b0 = static_cast<unsigned char>(input[i]);
    const bool has_b1 = i + 1 < input.size();
    const bool has_b2 = i + 2 < input.size();
    const unsigned char b1 = has_b1 ? static_cast<unsigned char>(input[i + 1]) : 0;
    const unsigned char b2 = has_b2 ? static_cast<unsigned char>(input[i + 2]) : 0;

    output.push_back(kBase64Table[(b0 >> 2) & 0x3F]);
    output.push_back(kBase64Table[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)]);
    output.push_back(has_b1 ? kBase64Table[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=');
    output.push_back(has_b2 ? kBase64Table[b2 & 0x3F] : '=');
  }

  return output;
}

}  // namespace radicc
