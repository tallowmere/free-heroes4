#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "h4/core/strong.hpp"
#include "h4/formats/scenario/records.hpp"
#include "h4/model/alignment.hpp"
#include "h4/model/skills.hpp"

namespace h4::model {

using HeroClassId                         = core::Strong<struct HeroClassIdTag, std::uint8_t>;
inline constexpr std::size_t kHeroClasses = 48;

inline constexpr std::size_t kHeroClassDisciplines = 5;

struct HeroClass {
    constexpr HeroClass(std::string_view keyword, std::optional<Alignment> alignment, bool magic, std::initializer_list<Discipline> starts) noexcept
    : keyword{keyword}, alignment{alignment}, magic{magic}, count_{starts.size()} {
        std::ranges::copy(starts, starts_.begin());
    }

    std::string_view keyword;
    std::optional<Alignment> alignment;
    bool magic = false;

    [[nodiscard]] constexpr std::span<const Discipline> disciplines() const noexcept {
        return std::span{starts_}.first(count_);
    }

private:
    std::array<Discipline, kHeroClassDisciplines> starts_{};
    std::size_t count_ = 0;
};

[[nodiscard]] std::span<const HeroClass> hero_classes() noexcept;
[[nodiscard]] const HeroClass* hero_class(HeroClassId id) noexcept;
[[nodiscard]] std::optional<HeroClassId> hero_class_id(const formats::HeroRecord& hero) noexcept;
[[nodiscard]] const HeroClass* hero_class(const formats::HeroRecord& hero) noexcept;
[[nodiscard]] std::optional<HeroClassId> hero_class_id(std::string_view keyword) noexcept;

[[nodiscard]] std::string hero_actor_name(const formats::HeroRecord& hero);

}
