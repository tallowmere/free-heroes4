#include "h4/model/scenario_folder.hpp"

#include <algorithm>
#include <format>
#include <system_error>
#include <tuple>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/model/scenario.hpp"

namespace h4::model {

namespace {

bool name_less(std::string_view a, std::string_view b) noexcept {
    return std::ranges::lexicographical_compare(a, b, [](char x, char y) {
        return core::ascii_lower(x) < core::ascii_lower(y);
    });
}

int rank(const ScenarioSummary& s, ScenarioOrder order) noexcept {
    switch (order) {
    case ScenarioOrder::name: return 0;
    case ScenarioOrder::size: return std::to_underlying(s.size);
    case ScenarioOrder::difficulty: return std::to_underlying(s.difficulty);
    case ScenarioOrder::players: return static_cast<int>(s.players);
    case ScenarioOrder::humans: return static_cast<int>(s.humans);
    case ScenarioOrder::allies: return static_cast<int>(s.alliances);
    case ScenarioOrder::map_type:
        return (s.edition ? -2 * std::to_underlying(*s.edition) : 2) + (s.multiplayer ? 1 : 0);
    }
    return 0;
}

}

core::Result<ScenarioFolder> ScenarioFolder::scan(const std::filesystem::path& dir, std::stop_token stop) {
    std::error_code ec;
    const std::filesystem::path absolute = std::filesystem::weakly_canonical(dir, ec);
    std::filesystem::directory_iterator it{ec ? dir : absolute, ec};
    if (ec)
        return core::fail(core::ErrorCode::io, std::format("{}: {}", dir.string(), ec.message()));

    ScenarioFolder folder;
    folder.path_ = absolute.empty() ? dir : absolute;
    for (const std::filesystem::directory_entry& entry : it) {
        if (stop.stop_requested())
            break;
        const std::string name = entry.path().filename().string();
        if (name.empty() || name.front() == '.')
            continue;
        if (entry.is_directory(ec)) {
            folder.subfolders_.push_back(name);
        } else if (entry.is_regular_file(ec) && core::iequals(entry.path().extension().string(), ".h4c")) {
            if (auto summary = read_scenario_summary(entry.path()))
                folder.scenarios_.push_back(std::move(*summary));
        }
    }
    std::ranges::sort(folder.subfolders_, name_less);
    return folder.sorted(ScenarioOrder::map_type);
}

ScenarioFolder ScenarioFolder::sorted(ScenarioOrder order, SortDirection direction) const {
    const auto less = [order](const ScenarioSummary& a, const ScenarioSummary& b) {
        const int ra = rank(a, order);
        const int rb = rank(b, order);
        if (ra != rb)
            return ra < rb;
        return name_less(a.name, b.name);
    };
    ScenarioFolder copy = *this;
    std::ranges::stable_sort(copy.scenarios_, [&](const ScenarioSummary& a, const ScenarioSummary& b) {
        return direction == SortDirection::ascending ? less(a, b) : less(b, a);
    });
    return copy;
}

bool ScenarioFolder::has_parent() const noexcept {
    return path_.has_parent_path() && path_.parent_path() != path_;
}

std::size_t ScenarioFolder::entry_count() const noexcept {
    return (has_parent() ? 1U : 0U) + subfolders_.size() + scenarios_.size();
}

FolderEntry ScenarioFolder::entry(std::size_t index) const {
    if (has_parent()) {
        if (index == 0)
            return {FolderEntryKind::parent, path_.parent_path(), {}, nullptr};
        --index;
    }
    if (index < subfolders_.size())
        return {FolderEntryKind::subfolder, path_ / subfolders_[index], subfolders_[index], nullptr};
    index -= subfolders_.size();
    const ScenarioSummary& s = scenarios_.at(index);
    return {FolderEntryKind::scenario, s.path, {}, &s};
}

std::optional<std::size_t> ScenarioFolder::first_scenario() const noexcept {
    if (scenarios_.empty())
        return std::nullopt;
    return entry_count() - scenarios_.size();
}

std::optional<std::size_t> ScenarioFolder::find(const std::filesystem::path& file) const {
    const auto it = std::ranges::find(scenarios_, file, &ScenarioSummary::path);
    if (it == scenarios_.end())
        return std::nullopt;
    return entry_count() - scenarios_.size() + static_cast<std::size_t>(it - scenarios_.begin());
}

}
