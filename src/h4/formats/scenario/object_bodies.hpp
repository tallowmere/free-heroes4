#pragma once

#include <cstdint>

#include "h4/formats/scenario/objects.hpp"

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

[[nodiscard]] std::uint16_t body_version(core::ByteReader& in, std::uint16_t max);
[[nodiscard]] std::uint8_t body_owner(core::ByteReader& in);

void read_army_body(core::ByteReader& in, RecordReader& records, ScenarioObject& out);

void read_object_body(core::ByteReader& in, RecordReader& records, ScenarioObject& out);

}
