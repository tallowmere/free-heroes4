#include "h4/core/byte_reader.hpp"

#include <algorithm>
#include <format>

namespace h4::core {

ByteReader::ByteReader(std::span<const std::byte> data, std::string_view context, std::size_t offset)
: data_{data}, context_{context} {
    if (offset > data_.size()) {
        position_ = data_.size();
        fail(ErrorCode::corrupt, std::format("starts at {} past the end", offset));
    } else {
        position_ = offset;
    }
}

bool ByteReader::require(std::size_t n) {
    if (!ok())
        return false;
    if (n > remaining()) {
        fail(ErrorCode::corrupt, std::format("truncated: {} bytes wanted, {} left", n, remaining()));
        return false;
    }
    return true;
}

std::uint8_t ByteReader::u8() {
    if (!require(1))
        return 0;
    return std::to_integer<std::uint8_t>(data_[position_++]);
}

std::uint16_t ByteReader::u16() {
    if (!require(2))
        return 0;
    const std::uint16_t value = le_u16(data_.subspan(position_).first<2>());
    position_ += 2;
    return value;
}

std::uint32_t ByteReader::u32() {
    if (!require(4))
        return 0;
    const std::uint32_t value = le_u32(data_.subspan(position_).first<4>());
    position_ += 4;
    return value;
}

std::uint64_t ByteReader::u64() {
    const std::uint64_t low = u32();
    return low | (std::uint64_t{u32()} << 32);
}

std::string_view ByteReader::string16() {
    const std::uint16_t n = u16();
    const auto span       = bytes(n);
    return {reinterpret_cast<const char*>(span.data()), span.size()};
}

std::span<const std::byte> ByteReader::bytes(std::size_t n) {
    if (!require(n))
        return {};
    const auto out = data_.subspan(position_, n);
    position_ += n;
    return out;
}

std::vector<std::uint8_t> ByteReader::u8_bytes(std::size_t n) {
    const auto span = bytes(n);
    std::vector<std::uint8_t> out(span.size());
    for (std::size_t i = 0; i < span.size(); ++i)
        out[i] = std::to_integer<std::uint8_t>(span[i]);
    return out;
}

void ByteReader::skip(std::size_t n) {
    if (require(n))
        position_ += n;
}

void ByteReader::seek(std::size_t position) {
    if (!ok())
        return;
    if (position > data_.size()) {
        fail(ErrorCode::corrupt, std::format("seek to {} past the end", position));
        return;
    }
    position_ = position;
}

std::size_t ByteReader::bounded_count(std::size_t count, std::size_t min_item_bytes) {
    if (!ok())
        return 0;
    if (min_item_bytes != 0 && count > remaining() / min_item_bytes) {
        fail(ErrorCode::corrupt, std::format("count {} cannot fit in {} remaining bytes", count, remaining()));
        return 0;
    }
    return count;
}

void ByteReader::fail(ErrorCode code, std::string_view message) {
    if (error_)
        return;
    error_.emplace(code, std::format("{}: {} (at byte {})", context_, message, position_));
}

bool starts_with_magic(std::span<const std::byte> bytes, std::span<const std::uint8_t> magic) noexcept {
    return bytes.size() >= magic.size() && std::ranges::equal(bytes.first(magic.size()), magic, [](std::byte b, std::uint8_t m) {
               return b == std::byte{m};
           });
}

}
