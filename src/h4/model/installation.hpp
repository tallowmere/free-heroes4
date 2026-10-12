#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace h4::io {
class ArchiveChain;
}

namespace h4::model {

enum class Expansion : std::uint8_t { gathering_storm, winds_of_war };

class InstalledContent {
public:
    constexpr InstalledContent() noexcept = default;

    [[nodiscard]] static InstalledContent from_archive_names(std::span<const std::string_view> names);
    [[nodiscard]] static InstalledContent detect(const io::ArchiveChain& chain);

    [[nodiscard]] constexpr InstalledContent with(Expansion e) const noexcept {
        InstalledContent copy = *this;
        copy.mask_            = static_cast<std::uint8_t>(copy.mask_ | bit(e));
        return copy;
    }
    [[nodiscard]] constexpr bool has(Expansion e) const noexcept {
        return (mask_ & bit(e)) != 0;
    }
    [[nodiscard]] constexpr bool has_any_expansion() const noexcept {
        return mask_ != 0;
    }

    friend constexpr bool operator==(InstalledContent, InstalledContent) noexcept = default;

private:
    [[nodiscard]] static constexpr std::uint8_t bit(Expansion e) noexcept {
        return static_cast<std::uint8_t>(1U << std::to_underlying(e));
    }

    std::uint8_t mask_ = 0;
};

}
