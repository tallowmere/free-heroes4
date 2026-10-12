#include "h4/formats/animation.hpp"

#include <format>
#include <numeric>

#include "h4/core/byte_reader.hpp"
#include "h4/core/clock.hpp"

namespace h4::formats {

namespace {
constexpr std::uint16_t kNoTrailerVersion = 11;
}

core::Result<Animation> Animation::parse(std::span<const std::byte> data) {
    auto embedded = BitmapGroup::parse_embedded(data, 0);
    if (!embedded)
        return std::unexpected{embedded.error()};

    Animation anim{std::move(embedded->group)};
    const std::size_t layers = anim.group_.layers().size();
    core::ByteReader reader{data, "animation", embedded->end};

    const std::uint16_t version = reader.u16();
    if (reader.ok() && version < kNoTrailerVersion) {
        anim.frame_ms_          = reader.u16();
        const std::size_t count = reader.bounded_count(reader.u32(), 4);
        anim.schedule_.reserve(count);
        for (std::size_t i = 0; i < count && reader.ok(); ++i) {
            const std::uint32_t index = reader.u32();
            if (index >= layers)
                reader.fail(core::ErrorCode::corrupt, std::format("frame {} schedules layer {} of {}", i, index, layers));
            anim.schedule_.push_back(index);
        }
        if (version >= 2) {
            anim.flag_a_  = reader.u8() != 0;
            anim.flag_b_  = reader.u8() != 0;
            anim.dword_a_ = reader.u32();
            anim.dword_b_ = reader.u32();
        }
    }

    if (anim.schedule_.empty()) {
        anim.schedule_.resize(layers);
        std::iota(anim.schedule_.begin(), anim.schedule_.end(), std::uint32_t{0});
    }
    return reader.finish(std::move(anim));
}

const Layer* Animation::layer_at(std::chrono::milliseconds at, std::uint64_t phase) const noexcept {
    if (schedule_.empty())
        return nullptr;
    const std::uint32_t layer = schedule_[core::cycle(at, std::chrono::milliseconds{frame_ms_}, phase, schedule_.size())];
    const auto layers = group_.layers();
    return layer < layers.size() ? &layers[layer] : nullptr;
}

}
