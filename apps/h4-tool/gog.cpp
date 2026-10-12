#include <cstdlib>
#include <filesystem>
#include <print>
#include <stop_token>
#include <string>

#include "h4/graphics/canvas.hpp"
#include "h4/graphics/surface565.hpp"
#include "h4/io/inno/installer.hpp"
#include "h4/io/inno/managed_install.hpp"
#include "h4/io/png_writer.hpp"
#include "h4/screens/boot/setup_view.hpp"
#include "tool.hpp"

namespace h4::tool {

int gog(Args args) {
    if (args.empty() || args.size() > 2)
        return usage();
    const std::filesystem::path exe{args[0]};
    const auto installer = io::Installer::open(exe);
    if (!installer)
        return fail(installer.error());

    if (args.size() == 1) {
        const auto engine = installer->files_under(io::kEngineDirectories);
        for (const io::InstallerFile& file : engine)
            std::println("{:>11} {:>3}  {}", file.size(), file.parts.size(), file.destination);
        std::println("game {}: {} files, {} for the engine", installer->game_id(), installer->files().size(), engine.size());
        return EXIT_SUCCESS;
    }

    io::ExtractProgress progress;
    const auto marker = io::install_from_installer(*installer, exe, std::filesystem::path{args[1]}, progress, std::stop_token{});
    if (!marker)
        return fail(marker.error());
    std::println("{} files, {} bytes into {}", marker->files, marker->bytes, args[1]);
    return EXIT_SUCCESS;
}

int setup_screen(Args args) {
    if (args.size() != 1 && args.size() != 2)
        return usage();
    const std::string_view state = args.size() == 2 ? args[1] : "";
    const auto built = screens::SetupView::build(state == "pick" || state == "ios" ? screens::SetupPrompt::pick_folder :
                                                                                     screens::SetupPrompt::drop,
                                                 state == "ios" || state == "download");
    if (!built)
        return fail(built.error());
    const auto& view = *built;
    if (state == "progress")
        view->show_progress(41, 123, 400, 1000);
    if (state == "download")
        view->show_download(412ULL << 20, 1081ULL << 20);
    graphics::Surface565 frame{screens::SetupView::kFrame};
    view->paint(graphics::Canvas{frame});
    if (auto written = io::write_png(std::string{args[0]}, frame.size(), frame.to_rgba()); !written)
        return fail(written.error());
    std::println("wrote {}", args[0]);
    return EXIT_SUCCESS;
}

}
