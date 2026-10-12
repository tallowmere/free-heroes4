#include "h4/formats/transition_set.hpp"

#include <array>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"

namespace h4::formats {

namespace {

constexpr std::string_view kContext = "transition set";

struct ColumnTable {
    std::array<std::uint32_t, kTransitionColumns + 1> offsets{};

    [[nodiscard]] constexpr std::uint32_t size() const noexcept {
        return offsets[kTransitionColumns];
    }
};

[[nodiscard]] constexpr ColumnTable column_table(std::size_t units_per_byte) noexcept {
    ColumnTable table;
    std::uint32_t at = 0;
    for (std::size_t c = 0; c < kTransitionColumns; ++c) {
        table.offsets[c]    = at;
        const std::size_t u = transition_column_units(c);
        at += static_cast<std::uint32_t>((u + units_per_byte - 1) / units_per_byte);
    }
    table.offsets[kTransitionColumns] = at;
    return table;
}

constexpr ColumnTable kBitColumns    = column_table(8);
constexpr ColumnTable kNibbleColumns = column_table(2);

[[nodiscard]] constexpr const ColumnTable& table_for(std::uint8_t kind) noexcept {
    return kind == 1 ? kBitColumns : kNibbleColumns;
}

constexpr std::array<std::string_view, kTransitionSlots> kSlotNames{
    "water_to_land 1", "water_to_land 1", "water_to_land 2", "water_to_land 2", "water_to_land 3",
    "land_to_water 1", "land_to_water 1", "land_to_water 2", "land_to_water 2", "land_to_water 3",
    "road 1",          "road 1",          "road 2",          "road 2",          "road 2",
    "land 1",          "land 2",          "land 2",          "land 3",          "land 3",
};

}

std::span<const std::string_view> transition_slot_names() noexcept {
    return kSlotNames;
}

std::uint32_t transition_code_key(std::uint32_t code) noexcept {
    std::uint32_t key   = 0;
    std::uint32_t scale = 1;
    for (int position = 0; position < 4; ++position, scale *= 10) {
        const std::uint32_t digit = (code / scale) % 10U;
        if (digit == 4)
            return 0x103U - static_cast<std::uint32_t>(position);
        key = (key << 2) | digit;
    }
    return key;
}

std::uint8_t TransitionEntry::coverage(std::size_t column, std::size_t unit) const noexcept {
    if (column >= kTransitionColumns || unit >= transition_column_units(column))
        return 0;
    const std::uint32_t base = table_for(kind_).offsets[column];
    if (kind_ == 1) {
        const std::size_t at = base + unit / 8;
        if (at >= data_.size())
            return 0;
        return ((data_[at] >> (unit & 7U)) & 1U) != 0 ? std::uint8_t{15} : std::uint8_t{0};
    }
    const std::size_t at = base + unit / 2;
    if (at >= data_.size())
        return 0;
    return static_cast<std::uint8_t>((data_[at] >> ((unit & 1U) * 4U)) & 0x0FU);
}

const TransitionSet* TransitionLibrary::by_name(std::string_view name) const noexcept {
    for (const TransitionSet& set : sets_)
        if (core::iequals(set.name(), name))
            return &set;
    return nullptr;
}

const TransitionSet* TransitionLibrary::slot(std::size_t id) const noexcept {
    return id < kSlotNames.size() ? by_name(kSlotNames[id]) : nullptr;
}

core::Result<TransitionLibrary> TransitionLibrary::parse(std::span<const std::byte> payload) {
    core::ByteReader in{payload, kContext};
    TransitionLibrary out;

    out.blob_ = in.u8_bytes(payload.size());
    in.seek(0);

    struct Raw {
        std::size_t offset = 0;
        std::uint32_t code = 0;
        std::uint8_t kind  = 0;
    };
    std::vector<std::vector<Raw>> raw;

    const std::size_t sets = in.bounded_count(in.u8(), 2 + 4);
    raw.reserve(sets);
    for (std::size_t s = 0; in.ok() && s < sets; ++s) {
        TransitionSet set;
        set.name_               = std::string{in.string16()};
        const std::size_t count = in.bounded_count(in.u32(), 5);
        std::vector<Raw> block;
        block.reserve(count);
        for (std::size_t e = 0; in.ok() && e < count; ++e) {
            Raw item;
            item.code = in.u32();
            item.kind = in.u8();
            if (item.kind != 1 && item.kind != 4) {
                in.fail(core::ErrorCode::corrupt, "transition entry kind");
                break;
            }
            item.offset = in.position();
            in.skip(table_for(item.kind).size());
            if (in.ok())
                block.push_back(item);
        }
        raw.push_back(std::move(block));
        out.sets_.push_back(std::move(set));
    }

    for (std::size_t s = 0; s < raw.size(); ++s) {
        out.sets_[s].entries_.reserve(raw[s].size());
        for (const Raw& item : raw[s]) {
            TransitionEntry entry;
            entry.code_ = item.code;
            entry.kind_ = item.kind;
            entry.data_ = std::span<const std::uint8_t>{out.blob_}.subspan(item.offset, table_for(item.kind).size());
            out.sets_[s].entries_.push_back(entry);
        }
    }

    return in.finish(std::move(out));
}

}
