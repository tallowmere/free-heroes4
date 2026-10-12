#include "h4/formats/adv_object.hpp"

#include <algorithm>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/packed_bits.hpp"

namespace h4::formats {

namespace {

constexpr std::string_view kContext = "adv_object";
constexpr std::int32_t kColumn      = 32;

[[nodiscard]] std::vector<std::uint8_t> read_bitset(core::ByteReader& in, std::size_t cells) {
    std::vector<std::uint8_t> out(cells, 0);
    const std::span<const std::byte> bytes = in.bytes((cells + 7) / 8);
    for (std::size_t i = 0; i < cells; ++i)
        out[i] = packed_bit(bytes, i) ? 1 : 0;
    return out;
}

[[nodiscard]] std::int32_t floor_div(std::int32_t a, std::int32_t b) noexcept {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}
[[nodiscard]] std::int32_t ceil_div(std::int32_t a, std::int32_t b) noexcept {
    return a >= 0 ? (a + b - 1) / b : -((-a) / b);
}

}

bool AdvObject::covers(std::int32_t x, std::int32_t y) const noexcept {
    if (x < 0 || y < 0 || x >= grid_w_ || y >= grid_h_)
        return false;
    return footprint_[static_cast<std::size_t>(x + grid_w_ * y)] != 0;
}

namespace {

[[nodiscard]] bool set_at(const std::vector<std::uint8_t>& grid, std::int32_t index) noexcept {
    return index >= 0 && static_cast<std::size_t>(index) < grid.size() && grid[static_cast<std::size_t>(index)] != 0;
}

}

bool AdvObject::passage_y(std::int32_t x, std::int32_t y) const noexcept {
    if (x < 0 || x >= grid_w_ || y < 0 || y > grid_h_)
        return false;
    return set_at(passage_y_, x + grid_w_ * y);
}

bool AdvObject::passage_x(std::int32_t x, std::int32_t y) const noexcept {
    if (x < 0 || x > grid_w_ || y < 0 || y >= grid_h_)
        return false;
    return set_at(passage_x_, y + grid_h_ * x);
}

bool AdvObject::wall_y(std::int32_t x, std::int32_t y, bool bridge) const noexcept {
    if (x < 0 || x >= grid_w_ || y < 0 || y > grid_h_)
        return false;
    return set_at(wall_y_, (x + grid_w_ * y) * 2 + (bridge ? 0 : 1));
}

bool AdvObject::wall_x(std::int32_t x, std::int32_t y, bool bridge) const noexcept {
    if (x < 0 || x > grid_w_ || y < 0 || y >= grid_h_)
        return false;
    return set_at(wall_x_, (y + grid_h_ * x) * 2 + (bridge ? 0 : 1));
}

bool AdvObject::touches(std::int32_t x, std::int32_t y) const noexcept {
    if (x < 0 || y < 0 || x >= grid_w_ || y >= grid_h_)
        return false;
    const bool covered = covers(x, y);
    return (passage_y(x, y) && (y == 0 || covered)) || (passage_x(x, y) && (x == 0 || covered)) ||
        (passage_y(x, y + 1) && (y + 1 == grid_h_ || covered)) || (passage_x(x + 1, y) && (x + 1 == grid_w_ || covered));
}

std::int32_t AdvObject::span_at(std::int32_t column) const noexcept {
    const std::int32_t at = column - span_first_;
    if (at < 0 || static_cast<std::size_t>(at) >= span_map_.size())
        return 0;
    return span_map_[static_cast<std::size_t>(at)];
}

void AdvObject::classify_layers() {
    layers_ = classify_sub_image(group_);

    frame_rect_ = frame_extent(group_, layers_, false).translated(anchor_);
}

core::Result<AdvObject> AdvObject::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, kContext};
    AdvObject out;

    out.version_ = in.u16();
    if (in.ok() && out.version_ > 6) {
        in.fail(core::ErrorCode::unsupported, "adv_object version");
        return in.finish(std::move(out), false);
    }
    (void)in.u8();

    std::uint16_t v3_flag = 1;
    if (out.version_ >= 3) {
        v3_flag = in.u16();
        if (in.ok() && v3_flag > 1)
            in.fail(core::ErrorCode::corrupt, "adv_object grid flag");
    }

    out.grid_w_ = in.u16();
    out.grid_h_ = in.u16();
    if (in.ok() && (out.grid_w_ < 1 || out.grid_w_ > 9 || out.grid_h_ < 1 || out.grid_h_ > 9))
        in.fail(core::ErrorCode::corrupt, "adv_object footprint size");
    if (!in.ok())
        return in.finish(std::move(out), false);

    const std::size_t w = static_cast<std::size_t>(out.grid_w_);
    const std::size_t h = static_cast<std::size_t>(out.grid_h_);
    out.footprint_      = read_bitset(in, w * h);
    if (v3_flag == 0)
        (void)read_bitset(in, w * h);
    else {
        out.passage_y_ = read_bitset(in, w * (h + 1));
        out.passage_x_ = read_bitset(in, (w + 1) * h);
    }
    if (out.version_ >= 6) {
        out.wall_y_ = read_bitset(in, 2 * w * (h + 1));
        out.wall_x_ = read_bitset(in, 2 * (w + 1) * h);
        out.deck_ = in.u8_bytes((w + 1) * (h + 1));
    }

    if (out.version_ >= 2) {
        if (const std::uint16_t zero = in.u16(); in.ok() && zero != 0)
            in.fail(core::ErrorCode::corrupt, "adv_object classification marker");
        for (std::size_t i = 0; i < out.names_.size(); ++i) {
            out.ids_[i]   = in.u16();
            out.names_[i] = std::string{in.string16()};
        }
    }

    if (out.version_ >= 4) {
        out.mode_ = in.u8();
        if (in.ok() && out.mode_ >= 3)
            in.fail(core::ErrorCode::corrupt, "adv_object mode");
    } else
        out.mode_ = (out.grid_w_ < 2 && out.grid_h_ < 2) ? 0 : 1;

    if (out.version_ >= 5 && in.u8() != 0) {
        const std::int32_t flag_x = in.i32();
        const std::int32_t flag_y = in.i32();
        out.flag_point_           = core::Point{flag_x, flag_y};
    }

    const std::int8_t tag = in.i8();
    if (in.ok() && tag != 0 && tag != 1) {
        in.fail(core::ErrorCode::corrupt, "adv_object sub-image tag");
        return in.finish(std::move(out), false);
    }
    out.overlay_ = tag == 0;

    bool header = false;
    if (out.overlay_) {
        const std::uint16_t sub_version = in.u16();
        if (in.ok() && sub_version >= 3)
            in.fail(core::ErrorCode::unsupported, "adv_object sub-image version");
        if (sub_version != 0)
            header = in.u16() != 0;
    } else if (in.u16() != 0)
        header = in.u16() != 0;

    std::uint16_t base_version = 0;
    if (header) {
        base_version = in.u16();
        if (base_version > 2) {
            (void)in.u16();
            (void)in.u16();
            (void)in.u16();
        }
    }
    if (!in.ok())
        return in.finish(std::move(out), false);

    auto body = read_sub_image_body(payload, in, header && base_version == 1);
    if (!body)
        return std::unexpected{body.error()};
    out.group_  = std::move(body->group);
    out.anchor_ = body->anchor;

    out.classify_layers();

    if (out.overlay_) {
        const std::size_t count = in.bounded_count(in.u32(), 2);
        const std::int32_t first = floor_div(out.frame_rect_.left(), kColumn);
        const std::int32_t last  = ceil_div(out.frame_rect_.right(), kColumn);
        if (in.ok() && static_cast<std::int32_t>(count) != last - first)
            in.fail(core::ErrorCode::corrupt, "adv_object span map column count");
        out.span_first_ = first;
        out.span_map_.reserve(count);
        for (std::size_t i = 0; in.ok() && i < count; ++i)
            out.span_map_.push_back(in.i16());
    }

    return in.finish(std::move(out));
}

bool AdvObject::is_bridge() const noexcept {
    return core::iequals(names_[0], "bridge");
}

}
