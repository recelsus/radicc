#pragma once

#include <string>
#include <string_view>

namespace radicc {

std::string encode_base64(std::string_view input);

}  // namespace radicc
