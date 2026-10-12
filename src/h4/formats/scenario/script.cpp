#include "h4/formats/scenario/script.hpp"

#include <cstdint>
#include <string_view>

#include "h4/core/ascii.hpp"
#include "h4/core/byte_reader.hpp"
#include "h4/formats/payload_version.hpp"
#include "h4/formats/scenario/records.hpp"

namespace h4::formats {

namespace {

constexpr int kMaxDepth = 64;
constexpr std::size_t kMinNodeBytes = 2;

class Walker {
public:
    Walker(core::ByteReader& in, std::vector<std::string>& messages, int depth, bool* removes = nullptr) noexcept
    : in_{&in}, messages_{&messages}, removes_{removes}, depth_{depth} {
    }

    void tree() {
        version_ = read_version(*in_, 0, 2, "script version");
        node();
    }

    void expression(std::uint16_t version) {
        version_ = version;
        bool_expr();
    }

private:
    void dispatch(void (Walker::*body)(std::string_view), const char* what) {
        if (++depth_ > kMaxDepth) {
            in_->fail(core::ErrorCode::corrupt, what);
            return;
        }
        const std::string_view name = in_->string16();
        if (in_->ok() && !name.empty())
            (this->*body)(name);
        --depth_;
    }

    void node() {
        dispatch(&Walker::action, "script nests too deeply");
    }

    void nodes(std::size_t count) {
        for (std::size_t i = 0; in_->ok() && i < count; ++i)
            node();
    }

    void player_byte() {
        const std::uint8_t v = in_->u8();
        if (in_->ok() && v > (version_ < 2 ? 7 : 8))
            in_->fail(core::ErrorCode::corrupt, "script player selector");
    }

    void skip(std::size_t bytes) {
        in_->skip(bytes);
    }

    [[nodiscard]] RecordReader records() {
        return RecordReader{*in_, messages_, depth_};
    }

    void artifact() {
        (void)records().artifact_instance();
    }

    void bool_expr() {
        dispatch(&Walker::bool_body, "expression nests too deeply");
    }

    void bool_body(std::string_view name) {
        using core::iequals;
        if (iequals(name, "true") || iequals(name, "false"))
            return;
        if (iequals(name, "not"))
            return bool_expr();
        if (iequals(name, "and") || iequals(name, "or")) {
            bool_expr();
            return bool_expr();
        }
        if (iequals(name, "==") || iequals(name, ">") || iequals(name, ">=") || iequals(name, "<") || iequals(name, "<=")) {
            num_expr();
            return num_expr();
        }
        if (iequals(name, "var") || iequals(name, "is_dead") || iequals(name, "is_imprisoned")) {
            (void)in_->string16();
            return;
        }
        if (iequals(name, "has_alignment"))
            return skip(2);
        if (iequals(name, "has_hero")) {
            skip(1);
            (void)in_->string16();
            return;
        }
        if (iequals(name, "give_skill?"))
            return skip(3);
        if (iequals(name, "has_artifact")) {
            if (in_->u8() != 0)
                player_byte();
            else
                skip(1);
            return artifact();
        }
        if (iequals(name, "is_alignment") || iequals(name, "is_color")) {
            player_byte();
            return skip(1);
        }
        if (iequals(name, "is_computer") || iequals(name, "is_eliminated") || iequals(name, "is_human"))
            return player_byte();
        if (iequals(name, "owns_hero") || iequals(name, "owns_town")) {
            player_byte();
            (void)in_->string16();
            return;
        }
        in_->fail(core::ErrorCode::corrupt, "unknown boolean expression");
    }

    void num_expr() {
        dispatch(&Walker::num_body, "expression nests too deeply");
    }

    void num_body(std::string_view name) {
        using core::iequals;
        if (iequals(name, "lit"))
            return skip(4);
        if (iequals(name, "rand"))
            return skip(8);
        if (iequals(name, "var")) {
            (void)in_->string16();
            return;
        }
        if (iequals(name, "player"))
            return player_byte();
        if (iequals(name, "materials")) {
            player_byte();
            return skip(1);
        }
        if (iequals(name, "creatures"))
            return skip(3);
        if (iequals(name, "total_creatures") || iequals(name, "total_heroes"))
            return skip(1);
        if (iequals(name, "exp_level"))
            return skip(1);
        if (iequals(name, "mastery"))
            return skip(2);
        if (iequals(name, "day") || iequals(name, "dow") || iequals(name, "week") || iequals(name, "month") || iequals(name, "wom"))
            return;
        if (iequals(name, "+") || iequals(name, "-") || iequals(name, "*") || iequals(name, "/") || iequals(name, "%")) {
            num_expr();
            return num_expr();
        }
        if (iequals(name, "neg"))
            return num_expr();
        in_->fail(core::ErrorCode::corrupt, "unknown numeric expression");
    }

    void action(std::string_view name);

    core::ByteReader* in_;
    std::vector<std::string>* messages_;
    bool* removes_         = nullptr;
    std::uint16_t version_ = 0;
    int depth_             = 0;
};

void Walker::action(std::string_view name) {
    using core::iequals;
    if (iequals(name, "text")) {
        messages_->emplace_back(in_->string16());
        if (version_ >= 1)
            nodes(in_->bounded_count(in_->u16(), kMinNodeBytes));
        return;
    }
    if (iequals(name, "seq"))
        return nodes(in_->bounded_count(in_->u16(), kMinNodeBytes));
    if (iequals(name, "if")) {
        bool_expr();
        node();
        return node();
    }
    if (iequals(name, "ask")) {
        (void)in_->string16();
        node();
        return node();
    }
    if (iequals(name, "combat")) {
        if (in_->u8() >= 3 && in_->ok()) {
            in_->fail(core::ErrorCode::corrupt, "combat kind");
            return;
        }
        ArmyRecord army;
        records().army(army);
        node();
        return node();
    }
    if (iequals(name, "build") || iequals(name, "change_owner"))
        return skip(1);
    if (iequals(name, "clear_lc_text") || iequals(name, "clear_l_msg") || iequals(name, "clear_vc_text") ||
        iequals(name, "clear_v_msg") || iequals(name, "disable_vc") || iequals(name, "enable_vc") ||
        iequals(name, "lose") || iequals(name, "win"))
        return player_byte();
    if (iequals(name, "dec_mana") || iequals(name, "inc_mana") || iequals(name, "dec_pop") || iequals(name, "inc_pop"))
        return skip(3);
    if (iequals(name, "dec_damage") || iequals(name, "inc_damage") || iequals(name, "dec_max_hp") ||
        iequals(name, "inc_max_hp") || iequals(name, "dec_max_mana") || iequals(name, "inc_max_mana") ||
        iequals(name, "dec_speed") || iequals(name, "inc_speed"))
        return skip(4);
    if (iequals(name, "dec_luck") || iequals(name, "inc_luck") || iequals(name, "dec_morale") ||
        iequals(name, "inc_morale") || iequals(name, "inc_move") || iequals(name, "inc_level"))
        return skip(2);
    if (iequals(name, "detonate") || iequals(name, "gosub")) {
        (void)in_->string16();
        return;
    }
    if (iequals(name, "give_artifact") || iequals(name, "rem_artifact")) {
        skip(1);
        for (std::size_t n = in_->bounded_count(in_->u16(), 2); in_->ok() && n-- > 0;)
            artifact();
        return;
    }
    if (iequals(name, "give_creature") || iequals(name, "take_creature"))
        return skip(5);
    if (iequals(name, "give_material") || iequals(name, "take_material"))
        return skip(29);
    if (iequals(name, "give_skill") || iequals(name, "inc_skill"))
        return skip(3);
    if (iequals(name, "give_spell"))
        return skip(3);
    if (iequals(name, "inc_exp"))
        return skip(5);
    if (iequals(name, "rem_event")) {
        if (removes_ != nullptr)
            *removes_ = true;
        return;
    }
    if (iequals(name, "no_op") || iequals(name, "rem_this"))
        return;
    if (iequals(name, "set_bool")) {
        (void)in_->string16();
        return bool_expr();
    }
    if (iequals(name, "set_num")) {
        (void)in_->string16();
        return num_expr();
    }
    if (iequals(name, "set_lc_text") || iequals(name, "set_l_msg") || iequals(name, "set_vc_text") || iequals(name, "set_v_msg")) {
        skip(1);
        (void)in_->string16();
        return;
    }
    in_->fail(core::ErrorCode::corrupt, "unknown script action");
}

}

void read_script(core::ByteReader& in, std::vector<std::string>& messages, int depth, bool* removes) {
    if (depth > kMaxDepth) {
        in.fail(core::ErrorCode::corrupt, "records nest too deeply");
        return;
    }
    Walker{in, messages, depth, removes}.tree();
}

void read_bool_expression(core::ByteReader& in, std::uint16_t version, int depth) {
    if (depth > kMaxDepth) {
        in.fail(core::ErrorCode::corrupt, "records nest too deeply");
        return;
    }
    std::vector<std::string> discarded;
    Walker{in, discarded, depth}.expression(version);
}

}
