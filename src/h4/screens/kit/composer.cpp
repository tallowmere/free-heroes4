#include "h4/screens/kit/composer.hpp"

#include <algorithm>
#include <array>
#include <format>

#include "h4/assets/assets.hpp"
#include "h4/assets/required.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/formats/placement_guide.hpp"
#include "h4/screens/kit/large_button.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

constexpr const auto& kBareFaces     = ui::ButtonFaces::kStateNames;
constexpr std::string_view kReleased = kBareFaces[0];

bool starts_with_part(std::string_view name, std::string_view prefix) noexcept {
    return name.size() > prefix.size() && core::istarts_with(name, prefix) && name[prefix.size()] == '_';
}

std::string_view lookup(const NameMap& names, std::string_view name, std::string_view fallback) {
    for (const auto& [key, value] : names)
        if (core::iequals(key, name))
            return value;
    return fallback;
}

bool placed_by_screen(const CompositionRules& rules, std::string_view name) {
    return std::ranges::any_of(rules.placed_by_screen, [name](std::string_view entry) {
        return core::iequals(name, entry) || starts_with_part(name, entry);
    });
}

std::optional<std::string_view> face_base(std::string_view name) {
    if (core::iequals(name, kReleased))
        return std::string_view{};
    const std::size_t base = name.size() - kReleased.size() - 1;
    if (name.size() > kReleased.size() && core::iends_with(name, kReleased) && name[base] == ui::ButtonFaces::kJoin)
        return name.substr(0, base);
    return std::nullopt;
}

bool belongs_to(std::string_view layer, std::string_view base) {
    if (!base.empty())
        return starts_with_part(layer, base);
    return std::ranges::any_of(kBareFaces, [layer](std::string_view face) { return core::iequals(layer, face); });
}

HelpDecision help_of(const assets::Assets& assets, const CompositionRules& rules, std::string_view name, std::string_view default_item) {
    const auto found       = std::ranges::find_if(rules.help_items,
                                                  [name](const HelpEntry& e) { return core::iequals(e.layer, name); });
    const HelpEntry* entry = found != rules.help_items.end() ? &*found : nullptr;
    const std::string_view window = entry != nullptr && !entry->window.empty() ? entry->window : rules.help_window;
    const std::string_view item   = entry != nullptr ? entry->item : default_item;
    if (window.empty() || item.empty())
        return HelpDecision::from_parent();
    return HelpDecision::row(assets, window, item);
}

struct Sorted {
    std::vector<const formats::Layer*> art;
    std::vector<std::string_view> bases;
    std::vector<const formats::Layer*> captioned;
    std::vector<const formats::Layer*> texts;
};

}

std::string name_stem(std::string_view name, std::string_view suffix) {
    if (core::iends_with(name, suffix))
        name.remove_suffix(suffix.size());
    if (!name.empty() && name.back() == '_')
        name.remove_suffix(1);
    return core::ascii_lowered(name);
}

ui::Button* Composition::button(std::string_view name) const noexcept {
    const auto it = buttons_.find(name);
    return it == buttons_.end() ? nullptr : it->second;
}

ui::Label* Composition::label(std::string_view name) const noexcept {
    const auto it = labels_.find(name);
    return it == labels_.end() ? nullptr : it->second;
}

ui::Sprite* Composition::sprite(std::string_view name) const noexcept {
    const auto it = sprites_.find(name);
    return it == sprites_.end() ? nullptr : it->second;
}

std::optional<core::Rect> Composition::guide(std::string_view name) const {
    const auto it = guides_.find(name);
    if (it == guides_.end())
        return std::nullopt;
    return it->second;
}

core::Result<ui::Label*> add_label(ui::Widget& parent,
                                   const assets::Assets& assets,
                                   const core::Rect& box,
                                   std::string words,
                                   graphics::TextAlignment alignment,
                                   const HelpDecision& help,
                                   std::int32_t font_size) {
    const auto font = assets::require_font(assets, font_size != 0 ? font_size : box.height());
    if (!font)
        return std::unexpected{font.error()};
    return &add_window<ui::Label>(parent, help, box, std::move(words), **font, ui::LabelStyle{graphics::TextStyle::engraved(), alignment});
}

namespace {

core::Result<Sorted>
sort_layers(const formats::BitmapGroup& group, const CompositionRules& rules, core::FoldedMap<core::Rect>& guides) {
    Sorted sorted;
    for (const formats::Layer& layer : group.layers()) {
        if (layer.is_placeholder())
            continue;
        const bool guide = formats::is_placement_guide(layer);
        if (guide)
            guides.emplace(std::string{layer.name()}, layer.bounds());
        if (placed_by_screen(rules, layer.name()))
            continue;
        if (guide) {
            if (core::iends_with(layer.name(), "button"))
                sorted.captioned.push_back(&layer);
            else if (core::iends_with(layer.name(), "text"))
                sorted.texts.push_back(&layer);
            continue;
        }
        if (const auto base = face_base(layer.name())) {
            if (base->empty() && !core::iequals(layer.name(), kBareFaces[0]))
                return core::fail(core::ErrorCode::corrupt, std::format("'{}': a button face with no button", layer.name()));
            sorted.bases.push_back(*base);
        }
        sorted.art.push_back(&layer);
    }
    return sorted;
}

void add_art(ui::Widget& parent, const Sorted& sorted, core::FoldedMap<ui::Sprite*>& sprites) {
    for (const formats::Layer* layer : sorted.art) {
        if (std::ranges::any_of(sorted.bases, [layer](std::string_view b) { return belongs_to(layer->name(), b); }))
            continue;
        auto& sprite = add_window<ui::Sprite>(parent, HelpDecision::from_parent(), *layer);
        sprite.set_name(std::string{layer->name()});
        sprites.emplace(std::string{layer->name()}, &sprite);
    }
}

core::Result<void> add_face_buttons(ui::Widget& parent,
                                    const assets::Assets& assets,
                                    const formats::BitmapGroup& group,
                                    const CompositionRules& rules,
                                    const Sorted& sorted,
                                    core::FoldedMap<ui::Button*>& buttons) {
    for (const std::string_view base : sorted.bases) {
        const auto faces = ui::ButtonFaces::from_group(group, base);
        if (!faces)
            return core::fail(core::ErrorCode::corrupt, std::format("'{}': its Released face is gone", base));
        auto& button = add_window<ui::Button>(parent, help_of(assets, rules, base, name_stem(base, "")), core::Point{}, *faces);
        button.set_name(std::string{base});
        buttons.emplace(std::string{base}, &button);
    }
    return {};
}

core::Result<void> add_captioned_buttons(ui::Widget& parent,
                                         const assets::Assets& assets,
                                         const CompositionRules& rules,
                                         const Sorted& sorted,
                                         core::FoldedMap<ui::Button*>& buttons) {
    for (const formats::Layer* guide : sorted.captioned) {
        const std::string default_key = name_stem(guide->name(), "button");
        const std::string_view key    = lookup(rules.text_keys, guide->name(), default_key);
        auto button = add_large_button(parent, assets, guide->bounds().origin(), key, help_of(assets, rules, guide->name(), default_key));
        if (!button)
            return std::unexpected{button.error()};
        (*button)->set_name(std::string{guide->name()});
        buttons.emplace(std::string{guide->name()}, *button);
    }
    return {};
}

core::Result<void> add_texts(ui::Widget& parent,
                             const assets::Assets& assets,
                             const CompositionRules& rules,
                             const Sorted& sorted,
                             core::FoldedMap<ui::Label*>& labels) {
    for (const formats::Layer* guide : sorted.texts) {
        const core::Rect box      = guide->bounds();
        const formats::Font* font = assets.font_for_size(box.height());
        if (font == nullptr)
            return core::fail(core::ErrorCode::not_found, std::format("'{}': no font for its text", guide->name()));
        const std::string default_key = name_stem(guide->name(), "text");
        const std::string words{assets.text(lookup(rules.text_keys, guide->name(), default_key))};
        auto& label = add_window<ui::Label>(parent,
                                            help_of(assets, rules, guide->name(), default_key),
                                            box,
                                            words,
                                            *font,
                                            ui::LabelStyle{graphics::TextStyle::engraved(), rules.text_alignment});
        label.set_name(std::string{guide->name()});
        labels.emplace(std::string{guide->name()}, &label);
    }
    return {};
}

}

core::Result<Composition>
compose(ui::Widget& parent, const assets::Assets& assets, const formats::BitmapGroup& group, const CompositionRules& rules) {
    Composition out;
    const auto sorted = sort_layers(group, rules, out.guides_);
    if (!sorted)
        return std::unexpected{sorted.error()};
    add_art(parent, *sorted, out.sprites_);
    if (auto made = add_face_buttons(parent, assets, group, rules, *sorted, out.buttons_); !made)
        return std::unexpected{made.error()};
    if (auto made = add_captioned_buttons(parent, assets, rules, *sorted, out.buttons_); !made)
        return std::unexpected{made.error()};
    if (auto made = add_texts(parent, assets, rules, *sorted, out.labels_); !made)
        return std::unexpected{made.error()};
    return out;
}

}
