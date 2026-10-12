#include "h4/assets/asset_library.hpp"

#include <array>
#include <format>

#include "h4/assets/asset_decode.hpp"
#include "h4/formats/raw_bitmap.hpp"
#include "h4/formats/table.hpp"
#include "h4/io/resource_source.hpp"

namespace h4::assets {

namespace {

std::string help_key(std::string_view window, std::string_view item) {
    return std::format("{}|{}", window, item);
}

template <class T, class Cache, class Load> const T* cached(Cache& cache, std::string_view key, Load load) {
    if (const auto it = cache.find(key); it != cache.end())
        return it->second.get();
    return cache.emplace(std::string{key}, load()).first->second.get();
}

template <class T, class Key, class Value>
void load_rows(const io::ResourceSource& source, const std::string& sheet, core::FoldedMap<T>& into, Key key, Value value) {
    auto bytes = source.read(sheet);
    if (!bytes)
        return;
    auto table = formats::Table::parse(*bytes);
    if (!table)
        return;
    for (std::size_t r = 0; r < table->row_count(); ++r)
        if (std::string k = key(*table, r); !k.empty())
            into.try_emplace(std::move(k), value(*table, r));
}

}

AssetLibrary::AssetLibrary(const io::ResourceSource& source) : source_{&source} {
    load_rows(
        source,
        "strings.Text.h4d",
        strings_,
        [](const formats::Table& table, std::size_t r) { return std::string{table.cell(r, 0)}; },
        [](const formats::Table& table, std::size_t r) { return std::string{table.cell(r, 1)}; }
    );
    load_rows(
        source,
        "table.Interface.h4d",
        helps_,
        [](const formats::Table& table, std::size_t r) {
            const std::string_view window = table.cell(r, 0);
            return window.empty() ? std::string{} : help_key(window, table.cell(r, 1));
        },
        [](const formats::Table& table, std::size_t r) {
            return std::array<std::string, 2>{std::string{table.cell(r, 2)}, std::string{table.cell(r, 3)}};
        }
    );
}

InterfaceHelp AssetLibrary::help(std::string_view window, std::string_view item) const {
    const auto it = helps_.find(help_key(window, item));
    if (it == helps_.end())
        return {};
    return {it->second[0], it->second[1]};
}

const formats::BitmapGroup* AssetLibrary::group(std::string_view name) const {
    return cached<formats::BitmapGroup>(groups_, name, [&] {
        return decode<formats::BitmapGroup>(*source_, entry_name("layers", name));
    });
}

const formats::TerrainTexture* AssetLibrary::terrain(std::string_view name) const {
    return cached<formats::TerrainTexture>(terrains_, name, [&] {
        return decode<formats::TerrainTexture>(*source_, entry_name("terrain", name));
    });
}

const formats::AdvObject* AssetLibrary::object(std::string_view name) const {
    return cached<formats::AdvObject>(objects_, name, [&] {
        return decode<formats::AdvObject>(*source_, entry_name("adv_object", name));
    });
}

const formats::AdvActor* AssetLibrary::actor(std::string_view name) const {
    return cached<formats::AdvActor>(actors_, name, [&] {
        return decode<formats::AdvActor>(*source_, entry_name("adv_actor", name));
    });
}

const formats::ActorSequence* AssetLibrary::sequence(std::string_view name) const {
    return cached<formats::ActorSequence>(sequences_, name, [&] {
        return decode<formats::ActorSequence>(*source_, entry_name("actor_sequence", name));
    });
}

const formats::CombatActor* AssetLibrary::combat_actor(std::string_view name) const {
    return cached<formats::CombatActor>(combat_actors_, name, [&] {
        return decode<formats::CombatActor>(*source_, entry_name("combat_actor", name));
    });
}

const formats::TransitionLibrary* AssetLibrary::transitions() const {
    if (!transitions_read_) {
        transitions_read_ = true;
        transitions_      = decode<formats::TransitionLibrary>(*source_, entry_name("transition", "Transitions"));
    }
    return transitions_.get();
}

const formats::Animation* AssetLibrary::animation(std::string_view name) const {
    return cached<formats::Animation>(animations_, name, [&] {
        return decode<formats::Animation>(*source_, entry_name("animation", name));
    });
}

const graphics::Surface565* AssetLibrary::painting(std::string_view name) const {
    return cached<graphics::Surface565>(paintings_, name, [&]() -> std::unique_ptr<const graphics::Surface565> {
        const auto raw = decode<formats::RawBitmap>(*source_, entry_name("bitmap_raw", name));
        return raw ? std::make_unique<const graphics::Surface565>(graphics::Surface565::from_raw_bitmap(*raw)) : nullptr;
    });
}

std::vector<std::byte> AssetLibrary::game_map(std::string_view name) const {
    auto bytes = source_->read(entry_name("game_maps", name));
    return bytes ? std::move(*bytes) : std::vector<std::byte>{};
}

const formats::Font* AssetLibrary::font(std::string_view entry) const {
    return cached<formats::Font>(fonts_, entry, [&] { return decode<formats::Font>(*source_, std::string{entry}); });
}

const formats::Font* AssetLibrary::font_for_size(std::int32_t asked) const {
    return font(kFontLadder[ladder_index(asked)].entry);
}

std::string_view AssetLibrary::text(std::string_view key) const {
    const auto it = strings_.find(key);
    return it == strings_.end() ? std::string_view{} : std::string_view{it->second};
}

}
