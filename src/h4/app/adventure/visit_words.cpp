#include "h4/app/adventure/visit_words.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "h4/app/adventure/adventure_panel.hpp"
#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/model/adventure_texts.hpp"
#include "h4/model/artifact_catalog.hpp"
#include "h4/model/map.hpp"
#include "h4/model/spell_catalog.hpp"
#include "h4/rules/game_state.hpp"
#include "h4/rules/treasure.hpp"

namespace h4::app {

namespace {

std::optional<screens::DialogItem> spell_item(const assets::Assets& assets, const model::Spell& spell) {
    if (!spell.school)
        return std::nullopt;
    const formats::BitmapGroup* frame = assets.group("button.frame_52");
    const formats::BitmapGroup* icons = assets.group(std::format("icons.spells.{}.52", model::school_keyword(*spell.school)));
    if (frame == nullptr || icons == nullptr)
        return std::nullopt;
    screens::DialogItem item{{}, spell.name, {.balloon = spell.name}};
    const formats::Layer* released = frame->find("released");
    const formats::Layer* guide    = frame->find("icon");
    const formats::Layer* icon     = icons->find(spell.name);
    if (icon != nullptr && guide != nullptr)
        item.pieces.push_back({icon, guide->bounds().origin() - icon->bounds().origin()});
    if (released != nullptr)
        item.pieces.push_back({released, {}});
    return item;
}

std::optional<screens::DialogItem> learner_item(const assets::Assets& assets, const formats::HeroRecord& hero) {
    const formats::BitmapGroup* rings = assets.group("creature_rings");
    if (rings == nullptr)
        return std::nullopt;
    screens::DialogItem item{{}, hero.name, {.balloon = hero.name}};
    const formats::Layer* portrait = rings->find("portrait");
    const formats::Layer* border   = rings->find("drag_border");
    const formats::Layer* face     = hero_face(assets, hero, 52);
    if (face != nullptr && portrait != nullptr)
        item.pieces.push_back({face, portrait->bounds().origin() - face->bounds().origin()});
    if (border != nullptr)
        item.pieces.push_back({border, {}});
    return item;
}

}

screens::BasicDialogContent shrine_words(const assets::Assets& assets,
                                         const model::AdventureTexts& texts,
                                         const model::MapActor& shrine,
                                         const model::Spell& spell,
                                         const rules::ShrineLesson& lesson,
                                         const model::MapActor& army) {
    const std::string school = spell.school ? model::school_word(*spell.school) : std::string{};
    const auto words         = [&](std::string_view keyword) {
        return model::substituted(texts.text(shrine.family, shrine.sub, {}, keyword),
                                  {{"%spell_level", std::to_string(spell.level)},
                                   {"%magic_type", school},
                                   {"%spell_name", spell.name},
                                   {"%skill_name", std::format("Basic {} Magic", school)}});
    };
    screens::BasicDialogContent content;
    content.title   = words("name");
    content.centred = true;
    content.text    = words(!lesson.learners.empty() ? "initial" : lesson.already ? "empty" : "denied");
    if (auto item = spell_item(assets, spell))
        content.items.push_back(std::move(*item));
    for (const std::uint8_t slot : lesson.learners)
        if (slot < army.army.slots.size())
            if (auto item = learner_item(assets, army.army.slots[slot].hero))
                content.items.push_back(std::move(*item));
    return content;
}

screens::TreasureChestContent
chest_words(const assets::Assets& assets, const model::AdventureTexts& texts, const model::MapActor& chest, std::int32_t gold) {
    const auto words = [&](std::string_view keyword) {
        return std::string{texts.text(chest.family, chest.sub, {}, keyword)};
    };
    return screens::TreasureChestContent{
        words("name"),
        words("Initial"),
        model::substituted(words("gold"), {{"%material", std::format("{} {}", gold, assets.text("gold"))}}),
        model::substituted(words("experience"), {{"%experience", std::to_string(rules::chest_experience(gold))}})
    };
}

screens::BasicDialogContent artifact_words(const assets::Assets& assets,
                                           const model::AdventureTexts& texts,
                                           const model::MapActor& chest,
                                           const model::ArtifactType& artifact) {
    screens::BasicDialogContent content;
    content.title = std::string{texts.text(chest.family, chest.sub, {}, "name")};
    content.text = model::substituted(texts.text(chest.family, chest.sub, {}, "Artifact"), {{"%an_artifact", artifact.article}});
    content.centred = true;
    screens::DialogItem item{{}, artifact.name, {.balloon = artifact.name}};
    if (const formats::Layer* icon = artifact_icon(assets, artifact); icon != nullptr)
        item.pieces.push_back({icon, {}});
    content.items.push_back(std::move(item));
    return content;
}

}
