#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "h4/formats/scenario/records.hpp"
#include "h4/model/artifact_catalog.hpp"

namespace h4::rules {

struct Artifact {
    formats::ArtifactInstance instance;
    std::int32_t worn = model::kBackpackSlot;

    friend bool operator==(const Artifact&, const Artifact&) = default;
};

struct Inventory {
    std::array<std::optional<Artifact>, formats::kWornSlots> equipment;
    std::vector<Artifact> backpack;

    friend bool operator==(const Inventory&, const Inventory&) = default;
};

namespace worn {
inline constexpr std::int32_t left_ring = 3, misc_1 = 4, misc_4 = 7, right_ring = 9, left_hand = 10, right_hand = 13,
                              both_hands = 15, miscellaneous = 18, ring = 19;
}

[[nodiscard]] Inventory receive(Inventory inventory, const Artifact& artifact);

[[nodiscard]] Inventory inventory_of(const formats::HeroRecord& hero, const model::ArtifactCatalog& artifacts);
[[nodiscard]] Artifact artifact_of(const formats::ArtifactInstance& instance, const model::ArtifactCatalog& artifacts);

}
