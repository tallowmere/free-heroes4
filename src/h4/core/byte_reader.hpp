#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::core {

class ByteReader final {
public:
    ByteReader(std::span<const std::byte> data, std::string_view context, std::size_t offset = 0);

    [[nodiscard]] std::uint8_t u8();
    [[nodiscard]] std::uint16_t u16();
    [[nodiscard]] std::uint32_t u32();
    [[nodiscard]] std::uint64_t u64();
    [[nodiscard]] std::int8_t i8() {
        return static_cast<std::int8_t>(u8());
    }
    [[nodiscard]] std::int16_t i16() {
        return static_cast<std::int16_t>(u16());
    }
    [[nodiscard]] std::int32_t i32() {
        return static_cast<std::int32_t>(u32());
    }

    [[nodiscard]] std::string_view string16();

    [[nodiscard]] std::span<const std::byte> bytes(std::size_t n);
    [[nodiscard]] std::vector<std::uint8_t> u8_bytes(std::size_t n);

    void skip(std::size_t n);
    void seek(std::size_t position);

    [[nodiscard]] std::size_t bounded_count(std::size_t count, std::size_t min_item_bytes);

    void fail(ErrorCode code, std::string_view message);

    [[nodiscard]] bool ok() const noexcept {
        return !error_.has_value();
    }
    [[nodiscard]] std::size_t position() const noexcept {
        return position_;
    }
    [[nodiscard]] std::size_t remaining() const noexcept {
        return data_.size() - position_;
    }
    [[nodiscard]] bool at_end() const noexcept {
        return position_ == data_.size();
    }

    template <class T> [[nodiscard]] Result<T> finish(T&& value, bool require_end = true) {
        if (ok() && require_end && !at_end())
            fail(ErrorCode::corrupt, std::to_string(remaining()) + " trailing bytes");
        if (error_)
            return std::unexpected<Error>{*error_};
        return std::forward<T>(value);
    }

    [[nodiscard]] std::optional<Error> error() const {
        return error_;
    }

private:
    [[nodiscard]] bool require(std::size_t n);

    std::span<const std::byte> data_;
    std::string_view context_;
    std::size_t position_ = 0;
    std::optional<Error> error_;
};

[[nodiscard]] constexpr std::uint16_t le_u16(std::span<const std::byte, 2> bytes) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(bytes[0]) | (std::to_integer<std::uint16_t>(bytes[1]) << 8));
}
[[nodiscard]] constexpr std::uint32_t le_u32(std::span<const std::byte, 4> bytes) noexcept {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i)
        value |= std::to_integer<std::uint32_t>(bytes[i]) << (8 * i);
    return value;
}

[[nodiscard]] bool starts_with_magic(std::span<const std::byte> bytes, std::span<const std::uint8_t> magic) noexcept;

[[nodiscard]] constexpr bool fits(std::uint64_t size, std::uint64_t offset, std::uint64_t length) noexcept {
    return offset <= size && length <= size - offset;
}

[[nodiscard]] inline std::optional<std::span<const std::byte>>
checked_subspan(std::span<const std::byte> bytes, std::uint64_t offset, std::uint64_t length) noexcept {
    if (!fits(bytes.size(), offset, length))
        return std::nullopt;
    return bytes.subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(length));
}

}
