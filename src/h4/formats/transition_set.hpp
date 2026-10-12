#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::formats {

inline constexpr std::size_t kTransitionColumns = 64;
inline constexpr std::size_t kTransitionEntries = 93;
inline constexpr std::size_t kTransitionSets    = 15;
inline constexpr std::size_t kTransitionSlots = 20;

[[nodiscard]] constexpr std::size_t transition_column_units(std::size_t column) noexcept {
    return column <= 31 ? column + 1 : kTransitionColumns - column;
}

[[nodiscard]] std::uint32_t transition_code_key(std::uint32_t code) noexcept;

class TransitionEntry {
public:
    [[nodiscard]] std::uint32_t code() const noexcept {
        return code_;
    }
    [[nodiscard]] std::uint32_t coverage_key() const noexcept {
        return transition_code_key(code_);
    }
    [[nodiscard]] std::uint8_t kind() const noexcept {
        return kind_;
    }

    [[nodiscard]] std::uint8_t coverage(std::size_t column, std::size_t unit) const noexcept;

    [[nodiscard]] std::span<const std::uint8_t> data() const noexcept {
        return data_;
    }

private:
    friend class TransitionLibrary;

    std::span<const std::uint8_t> data_;
    std::uint32_t code_ = 0;
    std::uint8_t kind_  = 0;
};

class TransitionSet {
public:
    [[nodiscard]] std::string_view name() const noexcept {
        return name_;
    }
    [[nodiscard]] std::span<const TransitionEntry> entries() const noexcept {
        return entries_;
    }
    [[nodiscard]] const TransitionEntry* entry(std::size_t index) const noexcept {
        return index < entries_.size() ? &entries_[index] : nullptr;
    }

private:
    friend class TransitionLibrary;

    std::string name_;
    std::vector<TransitionEntry> entries_;
};

class TransitionLibrary {
public:
    [[nodiscard]] static core::Result<TransitionLibrary> parse(std::span<const std::byte> payload);

    [[nodiscard]] std::span<const TransitionSet> sets() const noexcept {
        return sets_;
    }
    [[nodiscard]] const TransitionSet* by_name(std::string_view name) const noexcept;
    [[nodiscard]] const TransitionSet* slot(std::size_t id) const noexcept;

private:
    TransitionLibrary() = default;

    std::vector<std::uint8_t> blob_;
    std::vector<TransitionSet> sets_;
};

[[nodiscard]] std::span<const std::string_view> transition_slot_names() noexcept;

}
