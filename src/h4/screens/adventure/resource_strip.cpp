#include "h4/screens/adventure/resource_strip.hpp"

#include <utility>

#include "h4/screens/kit/builder.hpp"
#include "h4/ui/widgets/label.hpp"

namespace h4::screens {

ResourceStrip::ResourceStrip(Builder& root, const AdventureLayout& layout)
: Grouping{root, "the counters' words are the resource display's"} {
    Builder in = root.inside(*this);
    for (std::size_t i = 0; i < kAdventureResources; ++i) {
        const core::Rect guide = layout.resource_numbers[i];
        if (!guide.empty())
            counters_[i] = &in.label(guide, {}, ui::kEngravedCentred, HelpDecision::unread("the resource display's builder"));
    }
}

void ResourceStrip::set_amount(AdventureResource resource, std::string amount) {
    if (ui::Label* counter = counters_[std::to_underlying(resource)])
        counter->set_text(std::move(amount));
}

std::string_view ResourceStrip::amount(AdventureResource resource) const noexcept {
    const ui::Label* counter = counters_[std::to_underlying(resource)];
    return counter != nullptr ? counter->text() : std::string_view{};
}

}
