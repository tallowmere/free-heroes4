#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "h4/core/geometry.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/boot/check_list.hpp"
#include "h4/screens/kit/grouping.hpp"
#include "h4/ui/widgets/button.hpp"
#include "h4/ui/widgets/scrollbar.hpp"

namespace h4::ui {
class Scrollbar;
class ScrollView;
class ToggleButton;
}

namespace h4::screens {

struct PortCheckModel {
    std::string_view words;
    std::string_view balloon;
};

class OptionsColumn final : public Grouping {
public:
    OptionsColumn(Builder& builder, std::span<const CheckModel> retail_rows, std::span<const PortCheckModel> port_rows);

    [[nodiscard]] CheckList& retail() noexcept {
        return *retail_;
    }
    [[nodiscard]] const CheckList& retail() const noexcept {
        return *retail_;
    }
    void set_port_checked(std::size_t index, bool checked);
    [[nodiscard]] bool port_checked(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect port_check_rect(std::size_t index) const noexcept;
    [[nodiscard]] core::Rect scrollbar_rect() const noexcept;

    core::Signal<std::size_t, bool> port_toggled;

private:
    [[nodiscard]] std::int32_t add_port_section(Builder& sheet, std::span<const PortCheckModel> rows, const ui::ButtonFaces& box_faces);
    void add_scrollbar(Builder& in, const ui::ScrollbarArt& art, std::int32_t height);

    ui::ScrollView* view_     = nullptr;
    CheckList* retail_        = nullptr;
    ui::Scrollbar* scrollbar_ = nullptr;
    std::vector<ui::ToggleButton*> port_checks_;
    core::Connections connections_;
};

}
