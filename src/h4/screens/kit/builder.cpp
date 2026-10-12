#include "h4/screens/kit/builder.hpp"

#include <format>

#include "h4/assets/assets.hpp"
#include "h4/formats/bitmap_group.hpp"
#include "h4/formats/font.hpp"
#include "h4/screens/kit/large_button.hpp"
#include "h4/ui/widgets/sprite.hpp"

namespace h4::screens {

namespace {

const formats::Font kStandInFont = formats::Font::builtin();

}

struct Builder::State {
    const assets::Assets* assets;
    std::string_view help_window;
    std::string_view screen;
    const formats::Layer* stand_in;
    std::optional<core::Error> failure;
    Composition composition;

    void fail(core::ErrorCode code, std::string what) {
        if (!failure)
            failure = core::Error{code, std::move(what)};
    }
    [[nodiscard]] ui::ButtonFaces stand_in_faces() const noexcept {
        return ui::ButtonFaces{*stand_in, nullptr, nullptr, nullptr, nullptr};
    }
};

core::Result<Builder> Builder::open(ui::Widget& parent, const assets::Assets& assets, BuildSource source) {
    const formats::BitmapGroup* group = assets.group(source.group);
    if (group == nullptr)
        return core::fail(core::ErrorCode::not_found, std::format("{}: layers.{} missing", source.screen, source.group));
    if (group->layers().empty())
        return core::fail(core::ErrorCode::corrupt, std::format("{}: layers.{} is empty", source.screen, source.group));
    auto state = std::make_shared<State>(State{&assets, source.help_window, source.screen, &group->layers().front(), std::nullopt, {}});
    return Builder{std::move(state), parent, core::Point{}, *group, source.group};
}

core::Result<void> Builder::finish() const {
    if (state_->failure)
        return std::unexpected{state_->failure->with_context(state_->screen)};
    return {};
}

bool Builder::ok() const noexcept {
    return !state_->failure.has_value();
}

void Builder::fail(core::Error error) {
    if (!state_->failure)
        state_->failure = std::move(error);
}

const assets::Assets& Builder::assets() const noexcept {
    return *state_->assets;
}

const formats::BitmapGroup& Builder::group() const noexcept {
    return *group_;
}

const formats::BitmapGroup& Builder::group(std::string_view name) {
    if (const formats::BitmapGroup* found = state_->assets->group(name))
        return *found;
    state_->fail(core::ErrorCode::not_found, std::format("layers.{} missing", name));
    return *group_;
}

Builder Builder::within(ui::Widget& parent, core::Point origin) const {
    return Builder{state_, parent, origin, *group_, group_name_};
}

Builder Builder::inside(ui::Widget& child) const {
    return within(child, origin_ + child.rect().origin());
}

Builder Builder::within(ui::Widget& parent, std::string_view guide_name) {
    return within(parent, origin_ + guide(guide_name).origin());
}

Builder Builder::over(std::string_view group_name, ui::Widget& parent, core::Point origin) {
    if (const formats::BitmapGroup* other = state_->assets->group(group_name))
        return Builder{state_, parent, origin, *other, group_name};
    state_->fail(core::ErrorCode::not_found, std::format("layers.{} missing", group_name));
    return within(parent, origin);
}

Builder Builder::panel(core::Rect box, const HelpDecision& help) {
    auto& made = add<ui::Panel>(help, box);
    return within(made, origin_ + box.origin());
}

Builder Builder::centred(std::string_view name, const HelpDecision& help, std::optional<core::Size> size) {
    const core::Size shell = size.value_or(group_->bounds().size());
    auto& made             = add<ui::Panel>(help, core::Rect{{}, shell}.centred_in(parent_->rect()));
    made.set_name(std::string{name});
    return within(made, core::Point{});
}

core::Rect Builder::guide(std::string_view name) {
    const formats::Layer* found = group_->find(name);
    if (found == nullptr) {
        state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' guide", group_name_, name));
        return {};
    }
    return found->bounds().translated(core::Point{} - origin_);
}

const formats::Layer& Builder::layer(std::string_view name) {
    const formats::Layer* found = group_->find(name);
    if (found == nullptr) {
        state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' layer", group_name_, name));
        return *state_->stand_in;
    }
    return *found;
}

const formats::Layer& Builder::layer(std::string_view group_name, std::string_view name) {
    if (const formats::BitmapGroup* other = state_->assets->group(group_name)) {
        if (const formats::Layer* found = other->find(name))
            return *found;
        state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' layer", group_name, name));
    } else
        state_->fail(core::ErrorCode::not_found, std::format("layers.{} missing", group_name));
    return *state_->stand_in;
}

const formats::Layer* Builder::find(std::string_view name) const {
    return group_->find(name);
}

const formats::Font& Builder::font(std::int32_t size) {
    const formats::Font* found = state_->assets->font_for_size(size);
    if (found == nullptr) {
        state_->fail(core::ErrorCode::not_found, std::format("no font for size {}", size));
        return kStandInFont;
    }
    return *found;
}

ui::ButtonFaces Builder::faces(std::string_view base) {
    if (auto found = ui::ButtonFaces::from_group(*group_, base))
        return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no {}_Released", group_name_, base));
    return state_->stand_in_faces();
}

ui::ButtonFaces Builder::faces(std::string_view group, std::string_view base) {
    const formats::BitmapGroup* other = state_->assets->group(group);
    if (other == nullptr) {
        state_->fail(core::ErrorCode::not_found, std::format("layers.{} missing", group));
        return state_->stand_in_faces();
    }
    if (auto found = ui::ButtonFaces::from_group(*other, base))
        return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{} is incomplete", group));
    return state_->stand_in_faces();
}

ui::ScrollbarArt Builder::scrollbar_art(std::string_view group) {
    if (const formats::BitmapGroup* other = state_->assets->group(group))
        if (auto found = ui::ScrollbarArt::from_group(*other))
            return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{} is missing or incomplete", group));
    return {state_->stand_in_faces(), state_->stand_in_faces(), state_->stand_in, state_->stand_in};
}

ui::DialogFrameArt Builder::frame_art(std::string_view group) {
    if (const formats::BitmapGroup* other = state_->assets->group(group))
        if (auto found = ui::DialogFrameArt::from_group(*other))
            return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{} is missing or incomplete", group));
    const formats::Layer* s = state_->stand_in;
    return {s, s, s, s, s, s, s, s, s, s->bounds(), s->bounds()};
}

HelpDecision Builder::help(std::string_view item) const {
    return help(state_->help_window, item);
}

HelpDecision Builder::help(std::string_view window, std::string_view item) const {
    return HelpDecision::row(*state_->assets, window, item);
}

std::string Builder::text(std::string_view key) const {
    return std::string{state_->assets->text(key)};
}

void Builder::compose(const CompositionRules& rules) {
    if (origin_ != core::Point{}) {
        state_->fail(core::ErrorCode::invalid_argument, "compose places in the group's coordinates, not a sub-builder's");
        return;
    }
    auto composed = screens::compose(*parent_, *state_->assets, *group_, rules);
    if (!composed) {
        fail(composed.error());
        return;
    }
    state_->composition = std::move(*composed);
}

ui::Button& Builder::button(std::string_view name) {
    if (ui::Button* found = state_->composition.button(name))
        return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' button", group_name_, name));
    auto& stand_in = add<ui::Button>(HelpDecision::from_parent(), core::Point{}, state_->stand_in_faces());
    stand_in.set_visible(false);
    return stand_in;
}

ui::Label& Builder::text_window(std::string_view name) {
    if (ui::Label* found = state_->composition.label(name))
        return *found;
    state_->fail(core::ErrorCode::corrupt, std::format("layers.{}: no '{}' text window", group_name_, name));
    auto& stand_in = add<ui::Label>(HelpDecision::from_parent(), core::Rect{}, std::string{}, kStandInFont, ui::LabelStyle{});
    stand_in.set_visible(false);
    return stand_in;
}

ui::Button& Builder::large_button(std::string_view guide_name, std::string_view text_key, const HelpDecision& help) {
    return large_button(marker(guide_name), text_key, help);
}

ui::Button& Builder::large_button(core::Point at, std::string_view text_key, const HelpDecision& help) {
    auto made = add_large_button(*parent_, *state_->assets, at, text_key, help);
    if (made)
        return **made;
    fail(made.error());
    auto& stand_in = add<ui::Button>(HelpDecision::from_parent(), core::Point{}, state_->stand_in_faces());
    stand_in.set_visible(false);
    return stand_in;
}

ui::Label&
Builder::label(std::string_view guide_name, std::string words, const ui::LabelStyle& style, const HelpDecision& help, std::int32_t font_size) {
    return label(guide(guide_name), std::move(words), style, help, font_size);
}

ui::Label& Builder::label(core::Rect box, std::string words, const ui::LabelStyle& style, const HelpDecision& help, std::int32_t font_size) {
    const formats::Font& face = font(font_size != 0 ? font_size : box.height());
    return add<ui::Label>(help, box, std::move(words), face, style);
}

ui::Sprite& Builder::sprite(std::string_view name, const HelpDecision& help) {
    return sprite(layer(name), help);
}

ui::Sprite& Builder::sprite(const formats::Layer& art, const HelpDecision& help) {
    return add<ui::Sprite>(help, art, core::Point{} - origin_);
}

}
