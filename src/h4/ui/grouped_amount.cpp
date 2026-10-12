#include "h4/ui/grouped_amount.hpp"

#include <format>

namespace h4::ui {

std::string grouped_amount(std::int64_t amount) {
    std::uint64_t left = amount < 0 ? 0 - static_cast<std::uint64_t>(amount) : static_cast<std::uint64_t>(amount);
    std::string out;
    for (; left > 999; left /= 1000)
        out = std::format(",{:03}", left % 1000) + out;
    out = std::format("{}", left) + out;
    return amount < 0 ? "-" + out : out;
}

}
