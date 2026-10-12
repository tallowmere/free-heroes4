#include "h4/model/alignment.hpp"

#include "h4/core/ascii.hpp"

namespace h4::model {

std::string_view to_string(Alignment a) noexcept {
    switch (a) {
    case Alignment::life: return "life";
    case Alignment::order: return "order";
    case Alignment::death: return "death";
    case Alignment::chaos: return "chaos";
    case Alignment::nature: return "nature";
    case Alignment::might: return "might";
    }
    return {};
}

std::optional<Alignment> parse_alignment(std::string_view text) noexcept {
    for (const Alignment a : kAllAlignments)
        if (core::iequals(text, to_string(a)))
            return a;
    return std::nullopt;
}

}
