#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/core/geometry.hpp"
#include "h4/cursor/cursor_state.hpp"
#include "h4/ui/help.hpp"

namespace h4::graphics {
class Canvas;
}

namespace h4::ui {

class WidgetTree;

enum class PressVisual : std::uint8_t { released, highlighted, pressed };

class Widget {
public:
    virtual ~Widget();

    Widget(const Widget&)            = delete;
    Widget& operator=(const Widget&) = delete;
    Widget(Widget&&)                 = delete;
    Widget& operator=(Widget&&)      = delete;

    [[nodiscard]] core::Rect rect() const noexcept {
        return rect_;
    }
    void set_rect(core::Rect rect) noexcept {
        rect_ = rect;
    }

    [[nodiscard]] bool visible() const noexcept {
        return visible_;
    }
    void set_visible(bool visible) noexcept {
        visible_ = visible;
    }
    [[nodiscard]] bool shown() const noexcept;
    [[nodiscard]] bool enabled() const noexcept {
        return enabled_;
    }
    [[nodiscard]] bool hovered() const noexcept {
        return hovered_;
    }
    [[nodiscard]] bool held() const noexcept {
        return held_;
    }
    [[nodiscard]] PressVisual visual() const noexcept;
    void set_enabled(bool enabled);

    [[nodiscard]] std::string_view name() const noexcept {
        return name_;
    }
    void set_name(std::string name) {
        name_ = std::move(name);
    }

    [[nodiscard]] Widget* parent() noexcept {
        return parent_;
    }
    [[nodiscard]] const Widget* parent() const noexcept {
        return parent_;
    }

    template <class T, class... Args> T& emplace_child(Args&&... args) {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref     = *child;
        adopt(std::move(child));
        return ref;
    }

    [[nodiscard]] std::unique_ptr<Widget> remove_child(const Widget& child);
    void attach(std::unique_ptr<Widget> child);
    void drop_child(const Widget& child);
    void clear_children();
    void place_before(const Widget& child, const Widget& sibling);

    [[nodiscard]] std::span<const std::unique_ptr<Widget>> children() const noexcept {
        return children_;
    }

    void set_help(Help help) {
        help_ = std::move(help);
    }
    void set_help(HelpFunction source) {
        help_ = std::move(source);
    }
    [[nodiscard]] Help help_here(core::Point local) const;
    void declare_silent(Silence kind, std::string_view reason) noexcept {
        silence_ = kind;
        silent_  = reason;
    }
    void see_through() noexcept {
        see_through_ = true;
    }
    void popup_at_pointer() noexcept {
        popup_at_pointer_ = true;
    }
    void set_right_click(RightClickAction action) {
        right_click_ = std::move(action);
    }
    using PointerFunction = std::function<cursor::CursorState(core::Point local)>;
    void set_pointer(cursor::CursorState pointer) {
        pointer_ = pointer;
    }
    void set_pointer(PointerFunction source) {
        pointer_ = std::move(source);
    }

    [[nodiscard]] HelpAt help_at(core::Point point_in_parent) const;
    [[nodiscard]] cursor::CursorState pointer_at(core::Point point_in_parent) const;

    template <class Self> [[nodiscard]] auto* find(this Self& self, std::string_view name) noexcept {
        using Found = std::conditional_t<std::is_const_v<Self>, const Widget, Widget>;
        if (core::iequals(self.name_, name))
            return static_cast<Found*>(&self);
        for (const std::unique_ptr<Widget>& child : self.children_)
            if (Found* found = static_cast<Found&>(*child).find(name))
                return found;
        return static_cast<Found*>(nullptr);
    }

    [[nodiscard]] core::Point origin_in_root() const noexcept;
    [[nodiscard]] core::Rect rect_in_root() const noexcept;

    void paint(const graphics::Canvas& parent_canvas) const;

    [[nodiscard]] Widget* hit_test(core::Point point_in_parent) noexcept;

protected:
    explicit Widget(core::Rect rect) noexcept : rect_{rect} {
    }

    virtual void draw(const graphics::Canvas& ) const {
    }

    [[nodiscard]] virtual bool holds(core::Point ) const noexcept {
        return true;
    }

    void cancel_press();

    [[nodiscard]] virtual bool accepts_pointer() const noexcept {
        return false;
    }
    virtual void on_press(core::Point ) {
    }
    virtual void on_double_click(core::Point ) {
    }
    virtual void on_drag(core::Point ) {
    }
    virtual void on_pointer_moved(core::Point ) {
    }
    virtual void on_release(core::Point ) {
    }
    virtual void on_click() {
    }
    virtual void on_state_changed() {
    }

private:
    friend class WidgetTree;

    void adopt(std::unique_ptr<Widget> child);
    [[nodiscard]] const Widget* window_at(core::Point point_in_parent) const noexcept;
    void set_state(bool hovered, bool held);
    [[nodiscard]] std::vector<std::unique_ptr<Widget>>* graveyard() noexcept;

    core::Rect rect_;
    bool visible_ = true;
    bool enabled_ = true;
    bool hovered_ = false;
    bool held_    = false;
    std::vector<std::unique_ptr<Widget>>* graveyard_ = nullptr;
    std::string name_;
    std::variant<Help, HelpFunction> help_;
    RightClickAction right_click_;
    std::string_view silent_;
    Silence silence_       = Silence::none;
    bool popup_at_pointer_ = false;
    bool see_through_      = false;
    std::variant<std::monostate, cursor::CursorState, PointerFunction> pointer_;
    Widget* parent_ = nullptr;
    std::vector<std::unique_ptr<Widget>> children_;
    std::shared_ptr<const char> lifetime_ = std::make_shared<const char>('\0');
};

class Panel final : public Widget {
public:
    explicit Panel(core::Rect rect) noexcept : Widget{rect} {
    }
};

[[nodiscard]] core::Rect shown_rect(const Widget* widget) noexcept;

}
