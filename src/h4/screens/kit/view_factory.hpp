#pragma once

#include <expected>
#include <memory>
#include <type_traits>
#include <utility>

#include "h4/core/error.hpp"
#include "h4/core/geometry.hpp"

namespace h4::assets {
class Assets;
}

namespace h4::screens {

class ViewFactory {
public:
    template <class V, class... Content>
    [[nodiscard]] static core::Result<std::unique_ptr<V>> make(const assets::Assets& assets, core::Size frame, Content&&... content) {
        auto view = construct<V>(frame, std::forward<Content>(content)...);
        static_assert(std::is_same_v<decltype(view->compose(assets)), core::Result<void>>,
                      "a view composes as core::Result<void> compose(const assets::Assets&)");
        if (auto composed = view->compose(assets); !composed)
            return std::unexpected{composed.error()};
        return view;
    }

    template <class V, class... Content>
    [[nodiscard]] static core::Result<std::unique_ptr<V>> make_bare(core::Size frame, Content&&... content) {
        auto view = construct<V>(frame, std::forward<Content>(content)...);
        static_assert(std::is_same_v<decltype(view->compose()), core::Result<void>>,
                      "a bare view composes as core::Result<void> compose()");
        if (auto composed = view->compose(); !composed)
            return std::unexpected{composed.error()};
        return view;
    }

private:
    template <class V, class... Content>
    [[nodiscard]] static std::unique_ptr<V> construct(core::Size frame, Content&&... content) {
        static_assert(std::is_final_v<V>, "a view is final");
        return std::unique_ptr<V>{new V{frame, std::forward<Content>(content)...}};
    }
};

template <class V, class... Content>
[[nodiscard]] core::Result<std::unique_ptr<V>> compose_view(const assets::Assets& assets, core::Size frame, Content&&... content) {
    return ViewFactory::make<V>(assets, frame, std::forward<Content>(content)...);
}

template <class V, class... Content>
[[nodiscard]] core::Result<std::unique_ptr<V>> compose_view(core::Size frame, Content&&... content) {
    return ViewFactory::make_bare<V>(frame, std::forward<Content>(content)...);
}

}
