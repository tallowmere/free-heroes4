#pragma once

#include "h4/ui/help.hpp"

namespace h4::formats {
class Layer;
}

namespace h4::screens {

struct NamedIcon {
    const formats::Layer* base    = nullptr;
    const formats::Layer* overlay = nullptr;
    ui::Help help;
};

}
