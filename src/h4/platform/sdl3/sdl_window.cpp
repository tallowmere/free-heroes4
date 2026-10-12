#include "h4/platform/sdl3/sdl_window.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "h4/graphics/surface565.hpp"
#include "h4/platform/sdl3/sdl_cursors.hpp"
#include "h4/platform/sdl3/sdl_dialogs.hpp"
#include "h4/platform/sdl3/sdl_handle.hpp"
#include "h4/platform/sdl3/sdl_web_sign_in.hpp"

namespace h4::platform::sdl3 {

namespace {

class VideoSubsystem {
public:
    VideoSubsystem() noexcept : ok_{SDL_InitSubSystem(SDL_INIT_VIDEO)} {
    }
    VideoSubsystem(const VideoSubsystem&)            = delete;
    VideoSubsystem& operator=(const VideoSubsystem&) = delete;
    ~VideoSubsystem() {
        if (ok_)
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    [[nodiscard]] bool ok() const noexcept {
        return ok_;
    }

private:
    bool ok_;
};

void set_icon(SDL_Window* window, std::span<const std::byte> png) {
#if !defined(SDL_PLATFORM_IOS)
    if (png.empty())
        return;
    const SdlHandle<SDL_Surface> icon{SDL_LoadPNG_IO(SDL_IOFromConstMem(png.data(), png.size()),
                                                     true)};
    if (icon)
        SDL_SetWindowIcon(window, icon.get());
#else
    (void)window;
    (void)png;
#endif
}

core::Point to_point(float x, float y) noexcept {
    return {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
}

PointerButton to_button(Uint8 button) noexcept {
    return button == SDL_BUTTON_RIGHT ? PointerButton::secondary : PointerButton::primary;
}

Key to_key(SDL_Keycode key) noexcept {
    switch (key) {
    case SDLK_ESCAPE: return Key::escape;
    case SDLK_RETURN: return Key::enter;
    case SDLK_SPACE: return Key::space;
    default: return Key::other;
    }
}

#if defined(SDL_PLATFORM_IOS)
constexpr SDL_WindowFlags kWindowFlags = SDL_WINDOW_FULLSCREEN;
#else
constexpr SDL_WindowFlags kWindowFlags = SDL_WINDOW_RESIZABLE;
#endif

#if defined(SDL_PLATFORM_IOS) && TARGET_OS_SIMULATOR
constexpr SDL_PixelFormat kTextureFormat = SDL_PIXELFORMAT_XRGB8888;
#else
constexpr SDL_PixelFormat kTextureFormat = SDL_PIXELFORMAT_RGB565;
#endif

class SdlWindow final : public Window {
public:
    explicit SdlWindow(core::Size logical) : logical_{logical} {
    }

    core::Result<void> open(std::string_view title, std::span<const std::byte> icon_png) {
        SDL_SetHint(SDL_HINT_MOUSE_DPI_SCALE_CURSORS, "1");
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
        if (!video_.ok())
            return std::unexpected{sdl_error("SDL_InitSubSystem")};
        const std::string title_z{title};
        window_.reset(SDL_CreateWindow(title_z.c_str(), logical_.width, logical_.height, kWindowFlags | SDL_WINDOW_HIGH_PIXEL_DENSITY));
        if (!window_)
            return std::unexpected{sdl_error("SDL_CreateWindow")};
        set_icon(window_.get(), icon_png);
        renderer_.reset(SDL_CreateRenderer(window_.get(), nullptr));
        if (!renderer_)
            return std::unexpected{sdl_error("SDL_CreateRenderer")};
        SDL_SetRenderVSync(renderer_.get(), 1);
        if (!SDL_SetRenderLogicalPresentation(renderer_.get(), logical_.width, logical_.height, SDL_LOGICAL_PRESENTATION_LETTERBOX))
            return std::unexpected{sdl_error("SDL_SetRenderLogicalPresentation")};
        if (!rebuild_texture())
            return std::unexpected{sdl_error("SDL_CreateTexture")};
        cursors_.emplace(window_.get(), logical_);
        return {};
    }

    [[nodiscard]] core::Size logical_size() const noexcept override {
        return logical_;
    }

    [[nodiscard]] core::Size window_size() const noexcept override {
        int w = 0, h = 0;
        SDL_GetWindowSize(window_.get(), &w, &h);
        return {w, h};
    }

    void set_logical_size(core::Size size) override {
        if (size == logical_ || size.empty())
            return;
        logical_ = size;
        SDL_SetRenderLogicalPresentation(renderer_.get(), logical_.width, logical_.height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        (void)rebuild_texture();
        if (cursors_)
            cursors_->set_logical(logical_);
    }

    [[nodiscard]] std::vector<InputEvent> poll_events() override {
        std::vector<InputEvent> out;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            SDL_ConvertEventToRenderCoordinates(renderer_.get(), &e);
            switch (e.type) {
            case SDL_EVENT_QUIT: out.emplace_back(events::QuitRequested{}); break;
            case SDL_EVENT_MOUSE_MOTION:
                out.emplace_back(events::PointerMoved{to_point(e.motion.x, e.motion.y)});
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                out.emplace_back(events::PointerPressed{
                    to_point(e.button.x, e.button.y), to_button(e.button.button), e.button.clicks % 2 == 0
                });
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                out.emplace_back(events::PointerReleased{to_point(e.button.x, e.button.y), to_button(e.button.button)});
                break;
            case SDL_EVENT_DROP_FILE:
                if (e.drop.data != nullptr)
                    out.emplace_back(events::FileDropped{e.drop.data});
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat)
                    out.emplace_back(events::KeyPressed{to_key(e.key.key)});
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                if (cursors_)
                    cursors_->rescale();
                break;
            default: break;
            }
        }
        for (std::string& pick : dialogs_.take())
            out.emplace_back(events::FileDropped{std::move(pick)});
        for (std::string& landing : sign_in_.take())
            out.emplace_back(events::WebSignInFinished{std::move(landing)});
        return out;
    }

    void present(const graphics::Surface565& frame) override {
        if (!texture_)
            return;
        if (frame.size() == logical_)
            upload(frame);
        SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
        SDL_RenderClear(renderer_.get());
        SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer_.get());
    }

    void set_cursor(const CursorImage* image) override {
        if (cursors_)
            cursors_->show(image);
    }

    void open_file_dialog(std::string_view description, std::string_view extension) override {
        dialogs_.open(window_.get(), description, extension);
    }

    [[nodiscard]] bool offers_web_sign_in() const noexcept override {
        return WebSignIn::offered();
    }

    void open_web_sign_in(std::string_view url, std::string_view landing_prefix) override {
        sign_in_.open(window_.get(), url, landing_prefix);
    }

private:
    void upload(const graphics::Surface565& frame) {
        const int pitch = logical_.width * 2;
        if constexpr (kTextureFormat == SDL_PIXELFORMAT_RGB565) {
            SDL_UpdateTexture(texture_.get(), nullptr, frame.bits().data(), pitch);
        } else {
            void* pixels  = nullptr;
            int out_pitch = 0;
            if (!SDL_LockTexture(texture_.get(), nullptr, &pixels, &out_pitch))
                return;
            SDL_ConvertPixels(
                logical_.width, logical_.height, SDL_PIXELFORMAT_RGB565, frame.bits().data(), pitch, kTextureFormat, pixels, out_pitch
            );
            SDL_UnlockTexture(texture_.get());
        }
    }

    bool rebuild_texture() {
        texture_.reset(
            SDL_CreateTexture(renderer_.get(), kTextureFormat, SDL_TEXTUREACCESS_STREAMING, logical_.width, logical_.height)
        );
        if (!texture_)
            return false;
        SDL_SetTextureScaleMode(texture_.get(), SDL_SCALEMODE_NEAREST);
        return true;
    }

    core::Size logical_;
    VideoSubsystem video_;
    SdlHandle<SDL_Window> window_;
    SdlHandle<SDL_Renderer> renderer_;
    SdlHandle<SDL_Texture> texture_;
    std::optional<PointerCursors> cursors_;
    FileDialogs dialogs_;
    WebSignIn sign_in_;
};

}

core::Result<std::unique_ptr<Window>>
create_window(std::string_view title, core::Size logical_size, std::span<const std::byte> icon_png) {
    auto window = std::make_unique<SdlWindow>(logical_size);
    if (auto opened = window->open(title, icon_png); !opened)
        return std::unexpected{opened.error()};
    return window;
}

}
