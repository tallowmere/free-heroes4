#include "h4/screens/new_game/scenario_select_layout.hpp"

#include <format>
#include <string>

#include "h4/assets/required.hpp"
#include "h4/formats/bitmap_group.hpp"

namespace h4::screens {

core::Result<ScenarioSelectLayout> ScenarioSelectLayout::from_group(const formats::BitmapGroup& group) {
    ScenarioSelectLayout layout;
    const auto art = [&](std::string_view name, const formats::Layer*& out) -> core::Result<void> {
        auto layer = assets::require_layer(group, "dialog.new_game", name);
        if (!layer)
            return std::unexpected{layer.error()};
        out = *layer;
        return {};
    };
    const auto guide = [&](std::string_view name, core::Rect& out) -> core::Result<void> {
        auto layer = assets::require_layer(group, "dialog.new_game", name);
        if (!layer)
            return std::unexpected{layer.error()};
        out = (*layer)->bounds();
        return {};
    };

    core::Rect column_guide;
    for (auto step : {art("Background", layout.background),
                      art("Choose_File_Grid", layout.grid),
                      art("Description_Box", layout.description_box),
                      art("Vertical_Line", layout.vertical_line),
                      guide("Map_Title", layout.title),
                      guide("line_width", layout.line),
                      guide("Description", layout.description),
                      guide("Scrollbar", layout.scrollbar),
                      guide("column_width", column_guide)})
        if (!step)
            return std::unexpected{step.error()};
    for (std::size_t i = 0; i < kRows; ++i)
        if (auto step = guide(std::format("Map_{}", i + 1), layout.slots[i]); !step)
            return std::unexpected{step.error()};
    layout.column_width = column_guide.width();
    return layout;
}

core::Rect ScenarioSelectLayout::bounds() const noexcept {
    return background->bounds();
}

std::vector<ScenarioColumn> ScenarioColumnSet::shown() const {
    std::vector<ScenarioColumn> out;
    for (const ScenarioColumn column : kDataColumns)
        if (contains(column))
            out.push_back(column);
    return out;
}

core::Rect ScenarioSelectLayout::column(std::size_t index, std::size_t count) const noexcept {
    const auto from_right    = static_cast<std::int32_t>(count - index);
    const std::int32_t pitch = column_width + vertical_line->bounds().width();
    const std::int32_t left  = line.right() - from_right * pitch + vertical_line->bounds().width();
    return core::Rect::from_edges(left, title.top(), left + column_width, slots.back().bottom());
}

core::Rect ScenarioSelectLayout::divider(std::size_t index, std::size_t count) const noexcept {
    const core::Rect authored = vertical_line->bounds();
    return core::Rect{{column(index, count).left() - authored.width(), authored.top()}, authored.size()};
}

std::int32_t ScenarioSelectLayout::name_right(std::size_t count) const noexcept {
    return count == 0 ? line.right() : divider(0, count).left();
}

core::Rect ScenarioSelectLayout::row(std::size_t index) const noexcept {
    const core::Rect slot = slots[index];
    return core::Rect{{line.left(), slot.top()}, {line.width(), slot.height()}};
}

core::Rect ScenarioSelectLayout::wash() const noexcept {
    return core::Rect{{0, line.top() - slots[0].top()}, line.size()};
}

}
