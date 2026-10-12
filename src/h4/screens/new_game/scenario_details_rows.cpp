#include "h4/screens/new_game/scenario_details_rows.hpp"

#include <format>
#include <string_view>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/sprite.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

namespace {

constexpr std::array<std::string_view, ScenarioSelectLayout::kMaxColumns> kRowGuides{"size", "difficulty", "players", "humans", "allies", "map_type"};

}

ScenarioDetailsRows::ScenarioDetailsRows(Builder& builder, ScenarioColumnSet shown)
: Grouping{builder, "0x7AE290 helps each row's icon and box"} {
    Builder in                      = builder.inside(*this);
    const ui::ButtonFaces box_faces = in.faces("checkbox", "");
    for (std::size_t i = 0; i < kRowGuides.size(); ++i) {
        const std::string_view row = kRowGuides[i];
        const auto help            = in.help(row);
        in.sprite_at(std::format("{}_icon", row), in.faces("dialog.new_game", kColumnFaces[i]).released(), help);
        boxes_[i] = &in.toggle(std::format("{}_checkbox", row), box_faces, help, shown.contains(kDataColumns[i]));
    }
}

ScenarioColumnSet ScenarioDetailsRows::choice() const noexcept {
    ScenarioColumnSet out;
    for (std::size_t i = 0; i < boxes_.size(); ++i)
        out = out.with(kDataColumns[i], boxes_[i]->checked());
    return out;
}

core::Rect ScenarioDetailsRows::checkbox_rect(ScenarioColumn column) const noexcept {
    return column == ScenarioColumn::name ? core::Rect{} : boxes_[data_index(column)]->rect_in_root();
}

}
