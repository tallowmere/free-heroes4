#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

#include "h4/core/error.hpp"
#include "h4/model/scenario_summary.hpp"

namespace h4::model {

enum class ScenarioOrder : std::uint8_t { name, size, difficulty, players, humans, allies, map_type };

enum class SortDirection : std::uint8_t {
    ascending,
    descending
};

enum class FolderEntryKind : std::uint8_t { parent, subfolder, scenario };

struct FolderEntry {
    FolderEntryKind kind = FolderEntryKind::parent;
    std::filesystem::path target;
    std::string name;
    const ScenarioSummary* scenario = nullptr;
};

class ScenarioFolder {
public:
    [[nodiscard]] static core::Result<ScenarioFolder> scan(const std::filesystem::path& dir, std::stop_token stop = {});

    [[nodiscard]] ScenarioFolder sorted(ScenarioOrder order, SortDirection direction = SortDirection::ascending) const;

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }
    [[nodiscard]] bool has_parent() const noexcept;
    [[nodiscard]] const std::vector<std::string>& subfolders() const noexcept {
        return subfolders_;
    }
    [[nodiscard]] const std::vector<ScenarioSummary>& scenarios() const noexcept {
        return scenarios_;
    }

    [[nodiscard]] std::size_t entry_count() const noexcept;
    [[nodiscard]] FolderEntry entry(std::size_t index) const;
    [[nodiscard]] std::optional<std::size_t> first_scenario() const noexcept;
    [[nodiscard]] std::optional<std::size_t> find(const std::filesystem::path& file) const;

private:
    ScenarioFolder() = default;

    std::filesystem::path path_;
    std::vector<std::string> subfolders_;
    std::vector<ScenarioSummary> scenarios_;
};

}
