#include "h4/screens/new_game/player_row_list.hpp"

#include <format>
#include <string_view>

#include "h4/formats/bitmap_group.hpp"
#include "h4/screens/kit/builder.hpp"
#include "h4/screens/kit/control.hpp"
#include "h4/screens/kit/window_builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/sprite.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

PlayerRowList::PlayerRowList(Builder& builder)
: Grouping{builder, "0x7C1E20 helps each row's pieces"}, assets_{&builder.assets()},
  arrows_{&builder.group("button.arrows")}, flags_{&builder.group("icons.flags.38")},
  row_background_{&builder.layer("Player_Background")}, checkbox_faces_{builder.faces("checkbox", "")} {
    Builder in = builder.inside(*this);
    for (std::size_t i = 0; i < kRows; ++i) {
        RowWidgets& w = rows_[i];
        w.panel       = &in.add<ui::Panel>(HelpDecision::from_parent(), in.guide(std::format("Player_{}", i + 1)));
        w.panel->set_visible(false);
        Builder row = in.within(*w.panel, "Player_1");
        if (i == 0) {
            player_flag_   = row.marker("Player_Flag");
            team_flag_     = row.marker("Team_Flag");
            colour_arrows_ = row.marker("Color_Arrows");
        }
        build_row(row, i);
    }
}

void PlayerRowList::build_row(Builder& row, std::size_t index) {
    RowWidgets& w = rows_[index];
    const auto arrow = [&](std::string_view base) -> ui::Button& {
        return row.add<ui::Button>(row.help("alignment"),
                                   row.marker("Alignment_Arrows"),
                                   with_borrowed_disabled(row.faces("button.arrows", base), *arrows_, base));
    };
    w.alignment_up   = &arrow("Alignment_Up");
    w.alignment_down = &arrow("Alignment_Down");
    w.background     = &row.sprite(*row_background_, HelpDecision::from_parent());
    w.name  = &row.label("Player_Name", {}, ui::kEngravedLeft, row.help("player_name"));
    w.human = &row.toggle("human_checkbox", checkbox_faces_, HelpDecision::from_parent());
    connections_.relay(w.alignment_up->clicked, alignment_stepped, index, -1);
    connections_.relay(w.alignment_down->clicked, alignment_stepped, index, 1);
    connections_.relay(w.human->toggled, human_toggled, index);
}

void PlayerRowList::bind(std::span<const PlayerRowModel> rows) {
    for (std::size_t i = 0; i < kRows; ++i) {
        const bool live = i < rows.size();
        rows_[i].panel->set_visible(live);
        if (live)
            fill_row(i, rows[i]);
    }
}

void PlayerRowList::fill_row(std::size_t index, const PlayerRowModel& content) {
    RowWidgets& w = rows_[index];
    for (ui::Button* arrow : {w.alignment_up, w.alignment_down})
        arrow->set_enabled(content.alignment_arrows);
    set_banner(index, w.player_banner, w.player_emblem, player_flag_, content, content.alignment_emblem, "player_color_banner", "alignment_banner");
    set_colour_arrow(index, w.colour_up, true, content);
    set_colour_arrow(index, w.colour_down, false, content);
    set_banner(index, w.team_banner, w.team_emblem, team_flag_, content, content.team_emblem, "team_id", "team_id");
    w.name->set_text(content.name);
    w.human->set_checked(content.human);
    w.human->set_visible(content.human_capable);
    w.human->set_enabled(content.human_capable);
}

void PlayerRowList::set_banner(std::size_t index,
                               ui::Sprite*& sprite,
                               ui::Sprite*& emblem,
                               core::Point at,
                               const PlayerRowModel& content,
                               const std::string& emblem_name,
                               std::string_view colour_help,
                               std::string_view emblem_help) {
    ui::Widget& row             = *rows_[index].panel;
    const formats::Layer* layer = flags_->find(content.colour);
    const formats::Layer* mark  = flags_->find(emblem_name);
    if (layer == nullptr) {
        if (sprite != nullptr)
            sprite->set_visible(false);
        if (emblem != nullptr)
            emblem->set_visible(false);
        return;
    }
    if (sprite == nullptr) {
        sprite = &add_window<ui::Sprite>(row, HelpDecision::row(*assets_, "file_open", colour_help), *layer, at);
        row.place_before(*sprite, *rows_[index].background);
    } else {
        sprite->set_visible(true);
        sprite->set_layer(*layer, at);
    }
    const core::Point over_art = core::Point{} - layer->bounds().origin();
    if (emblem == nullptr) {
        if (mark == nullptr)
            return;
        emblem = &add_window<ui::Sprite>(*sprite, HelpDecision::row(*assets_, "file_open", emblem_help), *mark, over_art);
    } else {
        emblem->set_visible(mark != nullptr);
        if (mark != nullptr)
            emblem->set_layer(*mark, over_art);
    }
}

void PlayerRowList::set_colour_arrow(std::size_t index, ui::Button*& button, bool up, const PlayerRowModel& content) {
    auto faces = ui::ButtonFaces::from_group(*arrows_, std::format("{}_{}", content.colour, up ? "Up" : "Down"));
    if (faces)
        faces = with_borrowed_disabled(std::move(*faces), *arrows_, up ? "Alignment_Up" : "Alignment_Down");
    if (button == nullptr) {
        if (!faces)
            return;
        ui::Widget& row = *rows_[index].panel;
        button = &add_window<ui::Button>(row, HelpDecision::row(*assets_, "file_open", "player_color"), colour_arrows_, *faces);
        connections_.relay(button->clicked, colour_stepped, index, up ? -1 : 1);
        row.place_before(*button, *rows_[index].background);
    } else {
        button->set_visible(faces.has_value());
        if (faces)
            button->set_faces(*faces);
    }
    if (faces)
        button->set_enabled(content.colour_arrows);
}

core::Rect PlayerRowList::row_rect(std::size_t row) const noexcept {
    return row < kRows ? ui::shown_rect(rows_[row].panel) : core::Rect{};
}

core::Rect PlayerRowList::checkbox_rect(std::size_t row) const noexcept {
    return row < kRows ? ui::shown_rect(rows_[row].human) : core::Rect{};
}

core::Rect PlayerRowList::alignment_arrow_rect(std::size_t row, bool up) const noexcept {
    return row < kRows ? ui::shown_rect(up ? rows_[row].alignment_up : rows_[row].alignment_down) : core::Rect{};
}

core::Rect PlayerRowList::colour_arrow_rect(std::size_t row, bool up) const noexcept {
    return row < kRows ? ui::shown_rect(up ? rows_[row].colour_up : rows_[row].colour_down) : core::Rect{};
}

}
