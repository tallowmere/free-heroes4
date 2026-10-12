#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace h4::screens {

struct CompositionRules;

enum class ButtonArt : std::uint8_t {
    composed,
    faces,
    large,
    authored
};

struct ButtonSpec {
    std::string_view guide;
    std::string_view text_key = {};
    std::string_view help_window = {};
    std::string_view help_item   = {};
    ButtonArt art                = ButtonArt::composed;
    std::string_view faces     = {};
    std::string_view face_base = {};
    std::string_view unread = {};
};

void add_buttons(CompositionRules& rules, std::span<const ButtonSpec> table);

struct CaptionSpec {
    std::string_view guide;
    std::string_view text_key;
};

}
