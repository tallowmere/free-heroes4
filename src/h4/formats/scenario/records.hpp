#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace h4::core {
class ByteReader;
}

namespace h4::formats {

using SpellMask = std::bitset<192>;

struct ArtifactInstance {
    std::int16_t id    = -1;
    std::int16_t spell = -1;

    friend bool operator==(const ArtifactInstance&, const ArtifactInstance&) = default;
};

inline constexpr std::size_t kWornSlots = 14;
using Equipment                         = std::array<std::optional<ArtifactInstance>, kWornSlots>;

struct CreatureStack {
    std::int16_t creature = -1;
    std::int16_t count    = 0;
    std::vector<ArtifactInstance> carried;
};

inline constexpr std::size_t kHeroSkills = 36;
inline constexpr std::uint8_t kNoSkill   = 0xFF;

inline constexpr std::int8_t kHeroClasses = 48;

inline constexpr std::int32_t kCreatureCount = 74;
inline constexpr std::int32_t kHeroCount     = 312;
inline constexpr std::int32_t kArtifactCount = 249;

struct HeroRecord {
    std::int16_t id = -1;
    std::string name;
    std::int8_t hero_class = -1;
    std::int8_t sex        = -1;
    std::int8_t level = 1;
    std::array<std::uint16_t, 4> additions{};
    std::array<std::uint8_t, kHeroSkills> skills = filled_with_no_skill();
    SpellMask spells;
    Equipment equipment;
    std::vector<ArtifactInstance> backpack;

    [[nodiscard]] static constexpr std::array<std::uint8_t, kHeroSkills> filled_with_no_skill() noexcept {
        std::array<std::uint8_t, kHeroSkills> out{};
        for (std::uint8_t& rank : out)
            rank = kNoSkill;
        return out;
    }
};

struct ArmySlot {
    enum class Kind : std::uint8_t { empty, creature, hero };

    Kind kind = Kind::empty;
    CreatureStack stack;
    HeroRecord hero;

    [[nodiscard]] std::optional<std::uint8_t> creature_kind_id() const noexcept {
        if (kind != Kind::creature || stack.count <= 0 || stack.creature < 0 || stack.creature > 0xFF)
            return std::nullopt;
        return static_cast<std::uint8_t>(stack.creature);
    }

    [[nodiscard]] bool vacant() const noexcept {
        return kind == Kind::empty || (kind == Kind::creature && !creature_kind_id());
    }
};

struct ArmyRecord {
    std::array<ArmySlot, 7> slots{};

    [[nodiscard]] const HeroRecord* leader() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
};

struct NameSpec {
    enum class Kind : std::uint8_t {
        none,
        player,
        town
    };

    Kind kind           = Kind::none;
    std::uint8_t colour = 0;
    std::string town;
};

std::uint16_t read_marker(core::ByteReader& in, std::uint16_t max, std::string_view what);

class RecordReader {
public:
    RecordReader(core::ByteReader& in, std::vector<std::string>* words, int depth = 0) noexcept
    : in_{&in}, words_{words}, depth_{depth} {
    }

    void marker(std::uint16_t max = 0);

    ArtifactInstance artifact_instance();

    void army_slot(ArmySlot& out);
    void hero(HeroRecord& out);
    void army(ArmyRecord& out);

    void discrete_base();
    void attached_events(int count);
    void timed_events();
    void triggerable_events();
    void continuous_events();

    void spell_mask(SpellMask* out = nullptr);
    void disabled_skills();
    void name_spec(NameSpec& out);
    void quest_core();
    void script();

private:
    core::ByteReader* in_;
    std::vector<std::string>* words_;
    int depth_;
};

}
