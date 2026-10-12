#pragma once

#include <string_view>

#include "h4/screens/kit/grouping.hpp"

namespace h4::screens {

class Builder;
struct AdventureLayout;

inline constexpr std::string_view kStripPlate = "Creature Icon Holders";

class FrameChrome final : public Grouping {
public:
    FrameChrome(Builder& root, const AdventureLayout& layout);
};

}
