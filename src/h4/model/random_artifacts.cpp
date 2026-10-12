#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "h4/model/artifact_models.hpp"
#include "h4/model/map.hpp"
#include "h4/model/random_objects.hpp"
#include "h4/model/random_passes.hpp"

namespace h4::model {

namespace {

constexpr std::array<ArtifactId, 5> kTomes{ArtifactId{200}, ArtifactId{202}, ArtifactId{199}, ArtifactId{198}, ArtifactId{201}};
constexpr std::array<ArtifactId, 5> kStaves{ArtifactId{29}, ArtifactId{182}, ArtifactId{181}, ArtifactId{185}, ArtifactId{184}};

}

const ArtifactType* ArtifactPool::draw(Roller& roller, ArtifactRank rank, bool potion, std::span<const ArtifactId> fixed) {
    if (artifacts_ == nullptr)
        return nullptr;
    if (used_.empty())
        used_.assign(static_cast<std::size_t>(kArtifactIds), false);

    const auto wanted = [&](const ArtifactType& type) {
        if (!type.id || type.model.empty())
            return false;
        if (!fixed.empty())
            return std::ranges::find(fixed, *type.id) != fixed.end();
        if (potion)
            return type.potion();
        return type.rank == rank && !type.potion();
    };

    std::vector<const ArtifactType*> eligible;
    for (int relax = 0; relax < 3; ++relax) {
        eligible.clear();
        for (const ArtifactType& type : artifacts_->all()) {
            if (!wanted(type))
                continue;
            if (relax < 2 && !type.allowed)
                continue;
            if (relax < 1 && used_[type.id->value()])
                continue;
            eligible.push_back(&type);
        }
        if (eligible.empty())
            continue;
        const ArtifactType* picked = eligible[roller.next_index(eligible.size())];
        used_[picked->id->value()] = true;
        return picked;
    }
    return nullptr;
}

void ArtifactPool::resolve(MapActor& actor, Roller& roller) {
    const ArtifactType* picked = nullptr;
    if (actor.family == "random_artifact") {
        const std::optional<ArtifactRank> rank = parse_artifact_rank(actor.sub);
        if (!rank.has_value())
            return;
        picked = draw(roller, *rank, false, {});
    } else if (actor.family == "random_potion")
        picked = draw(roller, ArtifactRank::item, true, {});
    else if (actor.family == "random_tome")
        picked = draw(roller, ArtifactRank::item, false, kTomes);
    else if (actor.family == "random_staff")
        picked = draw(roller, ArtifactRank::item, false, kStaves);
    else
        return;
    if (picked == nullptr)
        return;

    actor.model  = std::string{picked->model};
    actor.family = "artifact";
    actor.sub    = picked->keyword;
}

}
