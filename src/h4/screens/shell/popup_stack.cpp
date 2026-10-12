#include "h4/screens/shell/popup_stack.hpp"

namespace h4::screens {

PopupStack::PopupStack(std::unique_ptr<Popup> base) noexcept : base_{std::move(base)}, host_{*base_} {
}

}
