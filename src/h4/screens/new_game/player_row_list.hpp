#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/kit/grouping.hpp"
#include "h4/ui/widgets/button.hpp"

namespace h4::assets {
class Assets;
}
namespace h4::formats {
class BitmapGroup;
class Layer;
}
namespace h4::screens {
class Builder;
}
namespace h4::ui {
class Label;
class Sprite;
class ToggleButton;
}

namespace h4::screens {

struct PlayerRowModel {
    std::string colour;
    std::string alignment_emblem;
    std::string team_emblem;
    std::string name;
    bool human            = false;
    bool human_capable    = false;
    bool colour_arrows    = false;
    bool alignment_arrows = false;
};

class PlayerRowList final : public Grouping {
public:
    static constexpr std::size_t kRows = 6;

    explicit PlayerRowList(Builder& builder);

    void bind(std::span<const PlayerRowModel> rows);

    [[nodiscard]] core::Rect row_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect checkbox_rect(std::size_t row) const noexcept;
    [[nodiscard]] core::Rect alignment_arrow_rect(std::size_t row, bool up) const noexcept;
    [[nodiscard]] core::Rect colour_arrow_rect(std::size_t row, bool up) const noexcept;

    core::Signal<std::size_t, int> alignment_stepped;
    core::Signal<std::size_t, int> colour_stepped;
    core::Signal<std::size_t, bool> human_toggled;

private:
    struct RowWidgets {
        ui::Widget* panel          = nullptr;
        ui::Button* alignment_up   = nullptr;
        ui::Button* alignment_down = nullptr;
        ui::Sprite* background     = nullptr;
        ui::Label* name            = nullptr;
        ui::ToggleButton* human    = nullptr;
        ui::Sprite* player_banner  = nullptr;
        ui::Sprite* player_emblem  = nullptr;
        ui::Button* colour_up      = nullptr;
        ui::Button* colour_down    = nullptr;
        ui::Sprite* team_banner    = nullptr;
        ui::Sprite* team_emblem    = nullptr;
    };

    void build_row(Builder& row, std::size_t index);
    void fill_row(std::size_t index, const PlayerRowModel& content);
    void set_banner(std::size_t index,
                    ui::Sprite*& sprite,
                    ui::Sprite*& emblem,
                    core::Point at,
                    const PlayerRowModel& content,
                    const std::string& emblem_name,
                    std::string_view colour_help,
                    std::string_view emblem_help);
    void set_colour_arrow(std::size_t index, ui::Button*& button, bool up, const PlayerRowModel& content);

    const assets::Assets* assets_         = nullptr;
    const formats::BitmapGroup* arrows_   = nullptr;
    const formats::BitmapGroup* flags_    = nullptr;
    const formats::Layer* row_background_ = nullptr;
    ui::ButtonFaces checkbox_faces_;
    core::Point player_flag_;
    core::Point team_flag_;
    core::Point colour_arrows_;
    std::array<RowWidgets, kRows> rows_{};
    core::Connections connections_;
};

}
