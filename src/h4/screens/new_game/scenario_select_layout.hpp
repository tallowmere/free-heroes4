#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::formats {
class BitmapGroup;
class Layer;
}

namespace h4::screens {

enum class ScenarioColumn : std::uint8_t { name, size, difficulty, players, humans, allies, map_type };

inline constexpr std::array<ScenarioColumn, 6> kDataColumns{ScenarioColumn::size,
                                                            ScenarioColumn::difficulty,
                                                            ScenarioColumn::players,
                                                            ScenarioColumn::humans,
                                                            ScenarioColumn::allies,
                                                            ScenarioColumn::map_type};

inline constexpr std::array<std::string_view, kDataColumns.size()> kColumnFaces{"Size", "Difficulty", "Players", "Humans", "Allies", "Map_Type"};

[[nodiscard]] constexpr std::size_t data_index(ScenarioColumn column) noexcept {
    return std::to_underlying(column) - 1;
}

class ScenarioColumnSet {
public:
    [[nodiscard]] static constexpr ScenarioColumnSet retail_default() noexcept {
        return ScenarioColumnSet{}
            .with(ScenarioColumn::size, true)
            .with(ScenarioColumn::difficulty, true)
            .with(ScenarioColumn::players, true)
            .with(ScenarioColumn::map_type, true);
    }

    [[nodiscard]] constexpr bool contains(ScenarioColumn column) const noexcept {
        return ((bits_ >> std::to_underlying(column)) & 1U) != 0;
    }
    [[nodiscard]] constexpr ScenarioColumnSet with(ScenarioColumn column, bool shown) const noexcept {
        const unsigned bit = 1U << std::to_underlying(column);
        ScenarioColumnSet out;
        out.bits_ = static_cast<std::uint8_t>(shown ? (bits_ | bit) : (bits_ & ~bit));
        return out;
    }
    [[nodiscard]] std::vector<ScenarioColumn> shown() const;

    friend constexpr bool operator==(ScenarioColumnSet, ScenarioColumnSet) noexcept = default;

private:
    std::uint8_t bits_ = 0;
};

struct ScenarioSelectLayout {
    static constexpr std::size_t kRows       = 10;
    static constexpr std::size_t kMaxColumns = kDataColumns.size();

    [[nodiscard]] static core::Result<ScenarioSelectLayout> from_group(const formats::BitmapGroup& group);

    [[nodiscard]] core::Rect bounds() const noexcept;
    [[nodiscard]] core::Rect column(std::size_t index, std::size_t count) const noexcept;
    [[nodiscard]] core::Rect divider(std::size_t index, std::size_t count) const noexcept;
    [[nodiscard]] std::int32_t name_right(std::size_t count) const noexcept;
    [[nodiscard]] core::Rect row(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect wash() const noexcept;

    const formats::Layer* background      = nullptr;
    const formats::Layer* grid            = nullptr;
    const formats::Layer* description_box = nullptr;
    const formats::Layer* vertical_line   = nullptr;

    core::Rect title;
    core::Rect line;
    core::Rect description;
    core::Rect scrollbar;
    std::array<core::Rect, kRows> slots;
    std::int32_t column_width = 0;
};

}
