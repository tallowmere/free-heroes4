#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "h4/app/controller.hpp"
#include "h4/app/installation_choice.hpp"
#include "h4/core/signal.hpp"
#include "h4/screens/boot/setup_view.hpp"

namespace h4::net {
class HttpClient;
}

namespace h4::app {

struct DropReading {
    enum class Kind : std::uint8_t {
        installation,
        installer,
        refused,
    };
    Kind kind = Kind::refused;
    std::filesystem::path path;
    std::string message;
};

[[nodiscard]] DropReading read_drop(const std::filesystem::path& dropped, const InstallationProbe& holds_installation);

struct SetupHooks {
    InstallationProbe holds_installation;
    std::function<void()> browse;
    std::function<void(const std::filesystem::path& data_dir)> installed;
    std::function<void()> sign_in;
    net::HttpClient* http = nullptr;
    std::filesystem::path download_dir;
};

class SetupController final : public Controller {
public:
    SetupController(std::unique_ptr<screens::SetupView> view, std::filesystem::path install_dir, SetupHooks hooks);
    ~SetupController() override;
    SetupController(const SetupController&)            = delete;
    SetupController& operator=(const SetupController&) = delete;

    [[nodiscard]] screens::View& view() noexcept override {
        return *view_;
    }
    [[nodiscard]] const screens::SetupView& setup_view() const noexcept {
        return *view_;
    }
    void on_key(platform::Key key) override;
    void on_file_dropped(const std::string& path) override;
    void on_web_sign_in(const std::string& landing) override;
    [[nodiscard]] core::Size preferred_frame(core::Size ) const override {
        return screens::SetupView::kFrame;
    }

private:
    void on_frame() override;
    struct Extraction;
    struct Download;

    [[nodiscard]] bool busy() const noexcept {
        return extraction_ || download_;
    }
    void start_download(std::string code);
    void start_extraction(const std::filesystem::path& exe);
    void cancel();
    void finish(const std::filesystem::path& data_dir);
    void step_download();
    void step_extraction();

    std::unique_ptr<screens::SetupView> view_;
    std::filesystem::path install_dir_;
    SetupHooks hooks_;
    core::Connections connections_;
    std::unique_ptr<Extraction> extraction_;
    std::unique_ptr<Download> download_;
    bool from_download_ = false;
};

}
