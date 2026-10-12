#pragma once

#include <array>
#include <string>
#include <string_view>

#include "h4/screens/adventure/adventure_layout.hpp"
#include "h4/screens/kit/grouping.hpp"

namespace h4::ui {
class Label;
}

namespace h4::screens {

class Builder;

class ResourceStrip final : public Grouping {
public:
    ResourceStrip(Builder& root, const AdventureLayout& layout);

    void set_amount(AdventureResource resource, std::string amount);
    [[nodiscard]] std::string_view amount(AdventureResource resource) const noexcept;

private:
    std::array<ui::Label*, kAdventureResources> counters_{};
};

}
