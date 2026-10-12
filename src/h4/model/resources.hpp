#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include "h4/core/strong.hpp"

namespace h4::model {

enum class Resource : std::uint8_t { gold, wood, ore, crystal, sulfur, mercury, gems };

inline constexpr std::size_t kResourceCount = 7;

[[nodiscard]] constexpr std::string_view resource_word(Resource resource) noexcept {
    constexpr std::array<std::string_view, kResourceCount> kWords{"gold", "wood", "ore", "crystal", "sulfur", "mercury", "gems"};
    return kWords[std::to_underlying(resource)];
}

inline constexpr std::array<std::string_view, kResourceCount>
    kPileSubs{"gold_pile", "wood_pile", "ore_pile", "crystal_pile", "sulfur_pile", "pot_of_mecury", "gem_pile"};
[[nodiscard]] constexpr std::string_view pile_sub(Resource resource) noexcept {
    return kPileSubs[std::to_underlying(resource)];
}
[[nodiscard]] constexpr std::optional<Resource> pile_resource(std::string_view sub) noexcept {
    for (std::size_t i = 0; i < kPileSubs.size(); ++i)
        if (kPileSubs[i] == sub)
            return static_cast<Resource>(i);
    return std::nullopt;
}

using ResourceAmount = core::Strong<struct ResourceAmountTag, std::int32_t, core::Additive, core::Scalable>;

class ResourceBundle {
public:
    constexpr ResourceBundle() noexcept = default;

    [[nodiscard]] constexpr ResourceAmount operator[](Resource r) const noexcept {
        return amounts_[std::to_underlying(r)];
    }

    [[nodiscard]] constexpr ResourceBundle with(Resource r, ResourceAmount v) const noexcept {
        ResourceBundle copy                  = *this;
        copy.amounts_[std::to_underlying(r)] = v;
        return copy;
    }

    [[nodiscard]] friend constexpr ResourceBundle operator+(ResourceBundle a, const ResourceBundle& b) noexcept {
        for (std::size_t i = 0; i < kResourceCount; ++i)
            a.amounts_[i] += b.amounts_[i];
        return a;
    }
    [[nodiscard]] friend constexpr ResourceBundle operator*(ResourceBundle a, std::int32_t n) noexcept {
        for (auto& v : a.amounts_)
            v = v * n;
        return a;
    }

    [[nodiscard]] constexpr bool covers(const ResourceBundle& cost) const noexcept {
        for (std::size_t i = 0; i < kResourceCount; ++i)
            if (amounts_[i] < cost.amounts_[i])
                return false;
        return true;
    }

    friend constexpr bool operator==(const ResourceBundle&, const ResourceBundle&) noexcept = default;

private:
    std::array<ResourceAmount, kResourceCount> amounts_{};
};

}
