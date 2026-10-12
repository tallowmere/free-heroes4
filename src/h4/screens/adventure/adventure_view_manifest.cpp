#include <format>
#include <string>
#include <vector>

#include "h4/assets/request.hpp"
#include "h4/screens/adventure/adventure_view.hpp"
#include "h4/screens/shell/resolution.hpp"

namespace h4::screens {

std::vector<assets::AssetRequest> AdventureView::assets_needed(core::Size frame) {
    using assets::AssetRequest;
    std::vector<AssetRequest> out{
        AssetRequest::group(std::format("adventure.{}", variant_suffix(variant_for_height(frame.height)))),
        AssetRequest::group("adventure.day_scroll"),
        AssetRequest::group("control.army_rings"),
        AssetRequest::group("creature_rings"),
        AssetRequest::group("town.small_list"),
        AssetRequest::group("town.random.89"),
        AssetRequest::group("icons.morale.34"),
        AssetRequest::every_font(),
    };
    for (const AdventureButton button : kAdventureButtons)
        out.push_back(AssetRequest::group(std::string{button_spec(button).faces}));
    return out;
}

}
