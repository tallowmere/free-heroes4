#pragma once

#include <cstddef>
#include <vector>

#include "h4/core/signal.hpp"

namespace h4::ui {

class ToggleButton;

class RadioGroup final {
public:
    explicit RadioGroup(std::vector<ToggleButton*> members, bool momentary = false);

    core::Signal<std::size_t> selection_changed;

    void select(std::size_t index);

    [[nodiscard]] std::size_t selected() const noexcept {
        return selected_;
    }

private:
    std::vector<ToggleButton*> members_;
    core::Connections connections_;
    std::size_t selected_ = 0;
    bool momentary_;
};

}
