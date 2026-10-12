#include "h4/model/army_member.hpp"

#include "h4/formats/scenario/records.hpp"
#include "h4/model/creature_catalog.hpp"
#include "h4/model/hero_class.hpp"

namespace h4::model {

std::optional<Alignment> member_alignment(const formats::ArmySlot& slot, const CreatureCatalog& creatures) {
    using Kind = formats::ArmySlot::Kind;
    if (slot.kind == Kind::hero) {
        const HeroClass* klass = hero_class(slot.hero);
        return klass == nullptr ? std::nullopt : klass->alignment;
    }
    const auto id = slot.creature_kind_id();
    if (!id.has_value())
        return std::nullopt;
    const CreatureType* type = creatures.find(CreatureKindId{*id});
    return type == nullptr ? std::nullopt : std::optional{type->alignment()};
}

}
