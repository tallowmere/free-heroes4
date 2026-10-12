#pragma once

#include <memory>

#include "h4/core/error.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/artifact_catalog.hpp"
#include "h4/model/creature_abilities.hpp"
#include "h4/model/creature_catalog.hpp"
#include "h4/model/hero_table.hpp"
#include "h4/model/skill_names.hpp"
#include "h4/model/spell_catalog.hpp"

namespace h4::io {
class ResourceSource;
}

namespace h4::model {

class GameDatabase {
public:
    [[nodiscard]] static core::Result<GameDatabase> load(const io::ResourceSource& source);

    [[nodiscard]] const CreatureCatalog& creatures() const noexcept {
        return *creatures_;
    }
    [[nodiscard]] const ArtifactCatalog& artifacts() const noexcept {
        return *artifacts_;
    }
    [[nodiscard]] const HeroTable& heroes() const noexcept {
        return *heroes_;
    }
    [[nodiscard]] const SpellCatalog& spells() const noexcept {
        return *spells_;
    }
    [[nodiscard]] const AdventureTexts& texts() const noexcept {
        return *texts_;
    }
    [[nodiscard]] const SkillNames& skills() const noexcept {
        return *skills_;
    }
    [[nodiscard]] const AbilityNames& abilities() const noexcept {
        return *abilities_;
    }

private:
    GameDatabase(std::shared_ptr<const CreatureCatalog> creatures,
                 std::shared_ptr<const ArtifactCatalog> artifacts,
                 std::shared_ptr<const HeroTable> heroes,
                 std::shared_ptr<const SpellCatalog> spells,
                 std::shared_ptr<const AdventureTexts> texts,
                 std::shared_ptr<const SkillNames> skills,
                 std::shared_ptr<const AbilityNames> abilities)
    : creatures_{std::move(creatures)}, artifacts_{std::move(artifacts)}, heroes_{std::move(heroes)},
      spells_{std::move(spells)}, texts_{std::move(texts)}, skills_{std::move(skills)}, abilities_{std::move(abilities)} {
    }

    std::shared_ptr<const CreatureCatalog> creatures_;
    std::shared_ptr<const ArtifactCatalog> artifacts_;
    std::shared_ptr<const HeroTable> heroes_;
    std::shared_ptr<const SpellCatalog> spells_;
    std::shared_ptr<const AdventureTexts> texts_;
    std::shared_ptr<const SkillNames> skills_;
    std::shared_ptr<const AbilityNames> abilities_;
};

}
