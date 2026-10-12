#pragma once

#include <cstdint>
#include <format>
#include <string_view>

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

[[nodiscard]] inline std::uint16_t
read_version(core::ByteReader& in, std::uint16_t oldest, std::uint16_t newest, std::string_view what) {
    const std::uint16_t version = in.u16();
    if (in.ok() && (version < oldest || version > newest))
        in.fail(core::ErrorCode::unsupported, std::format("{} {}", what, version));
    return version;
}

}
