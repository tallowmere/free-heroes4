#pragma once

#include "h4/formats/bitmap_group.hpp"
#include "h4/ui/widget.hpp"

namespace h4::ui {

class Sprite final : public Widget {
public:
    explicit Sprite(const formats::Layer& layer, core::Point offset = {}) noexcept
    : Widget{layer.bounds().translated(offset)}, layer_{&layer}, offset_{offset} {
    }

    void set_layer(const formats::Layer& layer, core::Point offset) noexcept {
        layer_  = &layer;
        offset_ = offset;
        set_rect(layer.bounds().translated(offset));
    }

protected:
    void draw(const graphics::Canvas& canvas) const override;
    [[nodiscard]] bool holds(core::Point local) const noexcept override {
        return layer_->transparency() == formats::Transparency::opaque || layer_->hit(local + layer_->bounds().origin());
    }

private:
    const formats::Layer* layer_;
    core::Point offset_;
};

}
