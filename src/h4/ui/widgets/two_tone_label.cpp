#include "h4/ui/widgets/two_tone_label.hpp"

#include "h4/ui/widgets/blend_window.hpp"

namespace h4::ui {

TwoToneLabel::TwoToneLabel(core::Rect rect,
                           std::string text,
                           const formats::Font& font,
                           LabelStyle plain,
                           LabelStyle washed,
                           std::optional<Wash> wash,
                           Mode mode)
: Widget{rect}, mode_{mode} {
    const core::Rect box{{}, rect.size()};
    plain_ = &emplace_child<Label>(box, text, font, plain);
    if (wash.has_value()) {
        wash_ = &emplace_child<BlendWindow>(box, wash->colour, wash->strength);
        hot_  = &wash_->emplace_child<Label>(box, std::move(text), font, washed);
    } else {
        hot_ = &emplace_child<Label>(box, std::move(text), font, washed);
    }
    refresh();
}

void TwoToneLabel::set_text(std::string text) {
    plain_->set_text(text);
    hot_->set_text(std::move(text));
}

void TwoToneLabel::set_washed(bool washed) {
    washed_ = washed;
    refresh();
}

void TwoToneLabel::on_state_changed() {
    if (mode_ == Mode::hover)
        refresh();
}

void TwoToneLabel::refresh() {
    if (mode_ == Mode::hover)
        washed_ = hovered() || held();
    plain_->set_visible(!washed_);
    if (wash_ != nullptr)
        wash_->set_visible(washed_);
    hot_->set_visible(washed_);
}

}
