#include "h4/screens/boot/options_column.hpp"

#include <string>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"
#include "h4/ui/widgets/scroll_view.hpp"
#include "h4/ui/widgets/scrollbar.hpp"
#include "h4/ui/widgets/toggle_button.hpp"

namespace h4::screens {

namespace {

constexpr std::string_view kSectionWords = "Port settings";

constexpr core::Rect kColumn{{375, 93}, {225, 232}};
constexpr std::int32_t kScrollbarLeft = 604;
constexpr std::int32_t kScrollStep = 38;

constexpr std::int32_t kRowHeight    = 20;
constexpr std::int32_t kBannerHeight = 17;
constexpr std::int32_t kWordsLeft = 36;
constexpr std::int32_t kWordsDrop = 4;
constexpr std::int32_t kSectionGap   = 16;
constexpr std::int32_t kBannerGap    = 6;
constexpr std::int32_t kPortRowGap   = 6;
constexpr std::int32_t kSheetPadding = 4;

}

OptionsColumn::OptionsColumn(Builder& builder, std::span<const CheckModel> retail_rows, std::span<const PortCheckModel> port_rows)
: Grouping{builder, "each row carries its own help"} {
    Builder in = builder.inside(*this);
    const ui::ScrollbarArt art      = in.scrollbar_art("vertical_scroll");
    const ui::ButtonFaces box_faces = in.faces("checkbox", "");

    view_ = &in.add<ui::ScrollView>(HelpDecision::from_parent(), kColumn, kScrollStep);
    view_->set_name("options_column");
    Builder sheet = in.inside(*view_).inside(view_->sheet());
    retail_       = &sheet.add<CheckList>(HelpDecision::from_parent(), sheet, retail_rows);

    const std::int32_t height = add_port_section(sheet, port_rows, box_faces);
    add_scrollbar(in, art, height);
}

std::int32_t OptionsColumn::add_port_section(Builder& sheet, std::span<const PortCheckModel> rows, const ui::ButtonFaces& box_faces) {
    std::int32_t y = retail_->rows_bottom() + kSectionGap;
    sheet
        .label(core::Rect{{0, y}, {kColumn.width(), kBannerHeight}}, std::string{kSectionWords}, ui::kEngravedCentred, HelpDecision::from_parent())
        .set_name("port_banner");
    y += kBannerHeight + kBannerGap;
    const std::int32_t face = box_faces.released().bounds().height();
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const core::Point art_at{retail_->box_left(), y};
        const auto help = HelpDecision::words({std::string{rows[i].balloon}, {}});
        sheet.label(core::Rect{{art_at.x + kWordsLeft, art_at.y + kWordsDrop}, {kColumn.width() - art_at.x - kWordsLeft, kRowHeight}},
                    std::string{rows[i].words},
                    ui::kEngravedLeft,
                    help);
        auto& box = sheet.toggle(art_at, box_faces, help);
        connections_.relay(box.toggled, port_toggled, i);
        port_checks_.push_back(&box);
        y += face + kPortRowGap;
    }
    return y + kSheetPadding - kPortRowGap;
}

void OptionsColumn::add_scrollbar(Builder& in, const ui::ScrollbarArt& art, std::int32_t height) {
    if (height <= kColumn.height())
        return;
    view_->set_content_height(height);
    scrollbar_ = &in.add<ui::Scrollbar>(
        HelpDecision::from_parent(), core::Point{kScrollbarLeft, kColumn.top()}, kColumn.height(), art, 0, view_->steps()
    );
    view_->attach(*scrollbar_);
}

void OptionsColumn::set_port_checked(std::size_t index, bool checked) {
    if (index < port_checks_.size())
        port_checks_[index]->set_checked(checked);
}

bool OptionsColumn::port_checked(std::size_t index) const noexcept {
    return index < port_checks_.size() && port_checks_[index]->checked();
}

core::Rect OptionsColumn::port_check_rect(std::size_t index) const noexcept {
    return index < port_checks_.size() ? ui::shown_rect(port_checks_[index]) : core::Rect{};
}

core::Rect OptionsColumn::scrollbar_rect() const noexcept {
    return ui::shown_rect(scrollbar_);
}

}
