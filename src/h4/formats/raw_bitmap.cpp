#include "h4/formats/raw_bitmap.hpp"

#include "h4/core/byte_reader.hpp"

namespace h4::formats {

namespace {
constexpr std::uint32_t kMaxSide = 16384;
}

core::Result<RawBitmap> RawBitmap::parse(std::span<const std::byte> data) {
    core::ByteReader reader{data, "bitmap_raw"};
    static_cast<void>(reader.u32());
    const std::uint32_t height = reader.u32();
    const std::uint32_t width  = reader.u32();
    const std::uint32_t bytes  = reader.u32();
    if (reader.ok() && (width == 0 || height == 0 || width > kMaxSide || height > kMaxSide))
        reader.fail(core::ErrorCode::corrupt, "implausible extents");
    if (reader.ok() && std::uint64_t{bytes} != std::uint64_t{width} * height * 3)
        reader.fail(core::ErrorCode::corrupt, "byte size disagrees with extents");

    const auto pixels = reader.bytes(bytes);
    std::vector<std::uint8_t> rgb(pixels.size());
    for (std::size_t i = 0; i + 2 < pixels.size(); i += 3) {
        rgb[i]     = std::to_integer<std::uint8_t>(pixels[i + 2]);
        rgb[i + 1] = std::to_integer<std::uint8_t>(pixels[i + 1]);
        rgb[i + 2] = std::to_integer<std::uint8_t>(pixels[i]);
    }

    const core::Size size{static_cast<std::int32_t>(width), static_cast<std::int32_t>(height)};
    return reader.finish(RawBitmap{size, std::move(rgb)});
}

}
