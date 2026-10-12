#include "h4/rules/inventory.hpp"

namespace h4::rules {

namespace {

using Equipment = std::array<std::optional<Artifact>, formats::kWornSlots>;

[[nodiscard]] bool free_slot(const Equipment& equipment, std::int32_t slot) {
    return !equipment[static_cast<std::size_t>(slot)].has_value();
}

[[nodiscard]] bool hands_allow(const Equipment& equipment, std::int32_t slot) {
    const auto& right = equipment[static_cast<std::size_t>(worn::right_hand)];
    return slot != worn::left_hand || !right.has_value() || right->worn != worn::both_hands;
}

[[nodiscard]] std::optional<std::int32_t> wear_slot(const Equipment& equipment, std::int32_t slot) {
    if (slot >= 0 && slot < static_cast<std::int32_t>(formats::kWornSlots)) {
        if (!free_slot(equipment, slot) || !hands_allow(equipment, slot))
            return std::nullopt;
        return slot;
    }
    switch (slot) {
    case worn::both_hands:
        if (!free_slot(equipment, worn::left_hand) || !free_slot(equipment, worn::right_hand))
            return std::nullopt;
        return worn::right_hand;
    case worn::miscellaneous:
        for (std::int32_t misc = worn::misc_1; misc <= worn::misc_4; ++misc)
            if (free_slot(equipment, misc))
                return misc;
        return std::nullopt;
    case worn::ring:
        if (free_slot(equipment, worn::right_ring))
            return worn::right_ring;
        if (free_slot(equipment, worn::left_ring))
            return worn::left_ring;
        return std::nullopt;
    default: return std::nullopt;
    }
}

}

Inventory receive(Inventory inventory, const Artifact& artifact) {
    if (const auto slot = wear_slot(inventory.equipment, artifact.worn))
        inventory.equipment[static_cast<std::size_t>(*slot)] = artifact;
    else
        inventory.backpack.insert(inventory.backpack.begin(), artifact);
    return inventory;
}

Artifact artifact_of(const formats::ArtifactInstance& instance, const model::ArtifactCatalog& artifacts) {
    const std::optional<model::ArtifactId> id = model::artifact_id_of(instance.id);
    const model::ArtifactType* type           = id ? artifacts.find(*id) : nullptr;
    return {instance, type != nullptr ? type->worn_slot() : model::kBackpackSlot};
}

Inventory inventory_of(const formats::HeroRecord& hero, const model::ArtifactCatalog& artifacts) {
    Inventory out;
    for (std::size_t slot = 0; slot < formats::kWornSlots; ++slot)
        if (hero.equipment[slot].has_value())
            out.equipment[slot] = artifact_of(*hero.equipment[slot], artifacts);
    for (const formats::ArtifactInstance& carried : hero.backpack)
        out.backpack.push_back(artifact_of(carried, artifacts));
    return out;
}

}
