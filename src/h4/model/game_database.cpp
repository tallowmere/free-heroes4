#include "h4/model/game_database.hpp"

#include <memory>

#include "h4/formats/table.hpp"
#include "h4/io/resource_source.hpp"

namespace h4::model {

namespace {

core::Result<formats::Table> load_table(const io::ResourceSource& source, std::string_view name) {
    auto bytes = source.read(name);
    if (!bytes)
        return std::unexpected{bytes.error()};
    auto table = formats::Table::parse(*bytes);
    if (!table)
        return std::unexpected{table.error().with_context(name)};
    return table;
}

template <class Catalog>
core::Result<std::shared_ptr<const Catalog>> load_catalog(const io::ResourceSource& source, std::string_view name) {
    auto table = load_table(source, name);
    if (!table)
        return std::unexpected{table.error()};
    auto catalog = Catalog::from_table(*table);
    if (!catalog)
        return std::unexpected{catalog.error()};
    return std::make_shared<const Catalog>(std::move(*catalog));
}

}

core::Result<GameDatabase> GameDatabase::load(const io::ResourceSource& source) {
    auto creatures = load_catalog<CreatureCatalog>(source, "table.creatures.h4d");
    if (!creatures)
        return std::unexpected{creatures.error()};
    auto artifacts = load_catalog<ArtifactCatalog>(source, "table.Artifacts.h4d");
    if (!artifacts)
        return std::unexpected{artifacts.error()};
    auto heroes = load_catalog<HeroTable>(source, "table.heroes.h4d");
    if (!heroes)
        return std::unexpected{heroes.error()};
    auto spells = load_catalog<SpellCatalog>(source, "table.Spells.h4d");
    if (!spells)
        return std::unexpected{spells.error()};
    auto texts = load_catalog<AdventureTexts>(source, "table.Adventure Object.h4d");
    if (!texts)
        return std::unexpected{texts.error()};
    auto skills = load_catalog<SkillNames>(source, "table.skills.h4d");
    if (!skills)
        return std::unexpected{skills.error()};
    auto abilities = load_catalog<AbilityNames>(source, "table.creature_abilities.h4d");
    if (!abilities)
        return std::unexpected{abilities.error()};

    return GameDatabase{std::move(*creatures),
                        std::move(*artifacts),
                        std::move(*heroes),
                        std::move(*spells),
                        std::move(*texts),
                        std::move(*skills),
                        std::move(*abilities)};
}

}
