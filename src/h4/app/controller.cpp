#include "h4/app/controller.hpp"

#include <format>
#include <string>

namespace h4::app {

Controller::~Controller() = default;

void Reporter::not_ported(std::string_view what) const {
    (*this)(std::format("{}: not ported yet", what));
}

}
