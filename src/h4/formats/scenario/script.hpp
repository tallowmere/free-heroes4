#pragma once

#include <string>
#include <vector>

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

void read_script(core::ByteReader& in, std::vector<std::string>& messages, int depth = 0, bool* removes = nullptr);

void read_bool_expression(core::ByteReader& in, std::uint16_t version, int depth = 0);

}
