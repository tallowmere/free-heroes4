#include "h4/formats/scenario/records.hpp"

#include <string_view>
#include <utility>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/packed_bits.hpp"
#include "h4/formats/payload_version.hpp"
#include "h4/formats/scenario/script.hpp"

namespace h4::formats {

namespace {

constexpr std::size_t kSpellMaskV0 = 23;
constexpr std::size_t kMinEventBytes = 6;

}

const HeroRecord* ArmyRecord::leader() const noexcept {
    for (const ArmySlot& slot : slots)
        if (slot.kind == ArmySlot::Kind::hero)
            return &slot.hero;
    return nullptr;
}

bool ArmyRecord::empty() const noexcept {
    for (const ArmySlot& slot : slots)
        if (slot.kind != ArmySlot::Kind::empty)
            return false;
    return true;
}

std::uint16_t read_marker(core::ByteReader& in, std::uint16_t max, std::string_view what) {
    const std::uint16_t value = in.u16();
    if (in.ok() && value > max)
        in.fail(core::ErrorCode::corrupt, what);
    return value;
}

void RecordReader::marker(std::uint16_t max) {
    read_marker(*in_, max, "record marker");
}

void RecordReader::script() {
    std::vector<std::string> discarded;
    read_script(*in_, words_ != nullptr ? *words_ : discarded, depth_ + 1);
}

ArtifactInstance RecordReader::artifact_instance() {
    const std::uint16_t id = in_->u16();
    if (in_->ok() && id >= kArtifactCount) {
        in_->fail(core::ErrorCode::corrupt, "artifact id");
        return {};
    }
    ArtifactInstance out{static_cast<std::int16_t>(id), -1};
    if (id == 124 || id == 166)
        out.spell = static_cast<std::int16_t>(in_->u16());
    return out;
}

void RecordReader::discrete_base() {
    script();
    (void)in_->string16();
    in_->skip(28);
}

void RecordReader::attached_events(int count) {
    for (int i = 0; in_->ok() && i < count; ++i) {
        marker();
        discrete_base();
        in_->skip(2);
    }
}

void RecordReader::timed_events() {
    for (std::size_t n = in_->bounded_count(in_->u16(), kMinEventBytes); in_->ok() && n-- > 0;) {
        marker();
        discrete_base();
        (void)in_->string16();
        in_->skip(6);
    }
}

void RecordReader::triggerable_events() {
    for (std::size_t n = in_->bounded_count(in_->u16(), kMinEventBytes); in_->ok() && n-- > 0;) {
        marker();
        discrete_base();
        (void)in_->string16();
        in_->skip(2);
    }
}

void RecordReader::continuous_events() {
    for (std::size_t n = in_->bounded_count(in_->u16(), kMinEventBytes); in_->ok() && n-- > 0;) {
        marker();
        script();
        (void)in_->string16();
        in_->skip(3);
    }
}

void RecordReader::spell_mask(SpellMask* out) {
    const std::size_t count                = in_->u8() == 0 ? kSpellMaskV0 : 24u;
    const std::span<const std::byte> bytes = in_->bytes(count);
    if (out == nullptr)
        return;
    for (std::size_t i = 0; i < bytes.size() * 8; ++i)
        if (packed_bit(bytes, i))
            out->set(i);
}

void RecordReader::disabled_skills() {
    if (in_->u8() != 0 && in_->ok()) {
        in_->fail(core::ErrorCode::corrupt, "skill set guard");
        return;
    }
    in_->skip(5);
}

void RecordReader::name_spec(NameSpec& out) {
    marker();
    const std::string_view kind = in_->string16();
    if (!in_->ok())
        return;
    if (core::iequals(kind, "player")) {
        out.kind   = NameSpec::Kind::player;
        out.colour = in_->u8();
        return;
    }
    if (core::iequals(kind, "town")) {
        out.kind = NameSpec::Kind::town;
        out.town = in_->string16();
        return;
    }
    out.kind = NameSpec::Kind::none;
}

void RecordReader::quest_core() {
    for (int i = 0; in_->ok() && i < 4; ++i)
        (void)in_->string16();
    script();
    const std::uint16_t version = in_->u16();
    read_bool_expression(*in_, version, depth_ + 1);
}

void RecordReader::hero(HeroRecord& out) {
    const std::uint16_t v = read_version(*in_, 8, 10, "hero record version");
    if (!in_->ok())
        return;
    const std::int16_t id = in_->i16();
    if (in_->ok() && id != -1 && (id < 0 || id >= kHeroCount)) {
        in_->fail(core::ErrorCode::corrupt, "hero id");
        return;
    }
    out.id = id;
    const std::uint8_t klass = in_->u8();
    const std::uint8_t sex   = in_->u8();
    if (in_->ok() && klass != 0xFF && klass >= kHeroClasses) {
        in_->fail(core::ErrorCode::corrupt, "hero class");
        return;
    }
    if (in_->ok() && sex != 0xFF && sex > 1) {
        in_->fail(core::ErrorCode::corrupt, "hero sex");
        return;
    }
    out.hero_class = klass == 0xFF ? std::int8_t{-1} : static_cast<std::int8_t>(klass);
    out.sex        = sex == 0xFF ? std::int8_t{-1} : static_cast<std::int8_t>(sex);
    if (v >= 5) {
        out.level = static_cast<std::int8_t>(in_->u8());
        if (v < 10) {
            out.additions[0] = in_->u16();
            out.additions[1] = in_->u16();
        } else {
            out.additions[0] = in_->u8();
            out.additions[1] = in_->u8();
        }
        out.additions[2] = in_->u8();
        out.additions[3] = in_->u8();
    }
    (void)in_->string16();
    out.name = std::string{in_->string16()};
    if (in_->u8() != 0) {
        const std::span<const std::byte> ranks = in_->bytes(kHeroSkills);
        for (std::size_t i = 0; i < out.skills.size() && i < ranks.size(); ++i)
            out.skills[i] = std::to_underlying(ranks[i]);
    }
    spell_mask(&out.spells);
    for (std::size_t i = 0; in_->ok() && i < kWornSlots; ++i)
        if (in_->u8() != 0)
            out.equipment[i] = artifact_instance();
    for (std::size_t n = in_->bounded_count(in_->u16(), 2); in_->ok() && n-- > 0;)
        out.backpack.push_back(artifact_instance());
    attached_events(3);
    timed_events();
    triggerable_events();
    continuous_events();
    if (v >= 9)
        in_->skip(1);
}

void RecordReader::army_slot(ArmySlot& out) {
    const std::uint8_t kind = in_->u8();
    if (!in_->ok() || kind == 0xFF)
        return;
    if (kind == 0) {
        const std::uint16_t v = read_version(*in_, 0, 1, "creature slot version");
        if (!in_->ok())
            return;
        const std::int16_t creature = in_->i16();
        if (in_->ok() && creature != -1 && (creature < 0 || creature >= kCreatureCount)) {
            in_->fail(core::ErrorCode::corrupt, "creature id");
            return;
        }
        out.kind           = ArmySlot::Kind::creature;
        out.stack.creature = creature;
        out.stack.count    = in_->i16();
        if (v >= 1)
            for (std::size_t n = in_->bounded_count(in_->u16(), 2); in_->ok() && n-- > 0;)
                out.stack.carried.push_back(artifact_instance());
        return;
    }
    if (kind != 1) {
        in_->fail(core::ErrorCode::corrupt, "army slot kind");
        return;
    }
    out.kind = ArmySlot::Kind::hero;
    hero(out.hero);
}

void RecordReader::army(ArmyRecord& out) {
    marker();
    for (ArmySlot& slot : out.slots) {
        if (!in_->ok())
            return;
        army_slot(slot);
    }
}

}
