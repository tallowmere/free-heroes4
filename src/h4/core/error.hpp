#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace h4::core {

enum class ErrorCode : std::uint8_t {
    io,
    corrupt,
    unsupported,
    not_found,
    invalid_argument,
};

[[nodiscard]] constexpr std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::io: return "io";
    case ErrorCode::corrupt: return "corrupt";
    case ErrorCode::unsupported: return "unsupported";
    case ErrorCode::not_found: return "not found";
    case ErrorCode::invalid_argument: return "invalid argument";
    }
    return "unknown";
}

class Error final {
public:
    Error(ErrorCode code, std::string message) : code_{code}, message_{std::move(message)} {
    }

    [[nodiscard]] ErrorCode code() const noexcept {
        return code_;
    }
    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

    [[nodiscard]] Error with_context(std::string_view context) const {
        return Error{code_, std::string{context} + ": " + message_};
    }

private:
    ErrorCode code_;
    std::string message_;
};

template <class T> using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> fail(ErrorCode code, std::string message) {
    return std::unexpected<Error>{Error{code, std::move(message)}};
}

template <class... Ts> [[nodiscard]] std::optional<Error> first_error(const Result<Ts>&... results) {
    std::optional<Error> out;
    ((out || results.has_value() || (out = results.error(), true)), ...);
    return out;
}

}
