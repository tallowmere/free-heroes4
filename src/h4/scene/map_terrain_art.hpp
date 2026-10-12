#pragma once

#include <array>
#include <cstdint>

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class TerrainTexture;
}

namespace h4::scene {

class TerrainArt {
public:
    static constexpr std::size_t kClasses  = 19;
    static constexpr std::size_t kVariants = 2;
    static constexpr std::size_t kSubs     = 2;
    static constexpr std::size_t kRoads    = 3;

    explicit TerrainArt(const assets::Assets& assets) noexcept : assets_{&assets} {
    }

    [[nodiscard]] const formats::TerrainTexture* ground(std::uint8_t terrain, std::uint8_t variant, std::uint8_t sub) const;
    [[nodiscard]] const formats::TerrainTexture* road(std::uint8_t road, std::uint8_t sub) const;

private:
    struct Entry {
        const formats::TerrainTexture* art = nullptr;
        bool resolved                      = false;
    };

    const assets::Assets* assets_;
    mutable std::array<Entry, kClasses * kVariants * kSubs> ground_{};
    mutable std::array<Entry, kRoads * kSubs> road_{};
};

}
