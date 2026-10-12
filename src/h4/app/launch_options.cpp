#include "h4/app/launch_options.hpp"

#include <charconv>
#include <format>

namespace h4::app {

std::string_view LaunchOptions::usage() noexcept {
    return "usage: openh4 [--data <Heroes4/Data>] [--resolution "
           "800|1024|1280]\n"
           "       (H4_DATA_DIR is used when --data is absent; without either, "
           "the\n"
           "        installation chosen before, or the first-boot setup)\n";
}

std::filesystem::path maps_dir_of(const std::filesystem::path& data_dir) {
    std::filesystem::path data = data_dir;
    if (!data.has_filename())
        data = data.parent_path();
    return data.parent_path() / "maps";
}

core::Result<LaunchOptions> LaunchOptions::parse(std::span<const std::string_view> args, const char* env_data_dir) {
    LaunchOptions options;
    if (env_data_dir != nullptr)
        options.data_dir = env_data_dir;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        const auto value           = [&]() -> core::Result<std::string_view> {
            if (i + 1 >= args.size())
                return core::fail(core::ErrorCode::invalid_argument, std::format("{} needs a value", arg));
            return args[++i];
        };

        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        } else if (arg == "--data") {
            auto v = value();
            if (!v)
                return std::unexpected{v.error()};
            options.data_dir = *v;
        } else if (arg == "--resolution") {
            auto v = value();
            if (!v)
                return std::unexpected{v.error()};
            std::int32_t width    = 0;
            const auto [end, ec]  = std::from_chars(v->data(), v->data() + v->size(), width);
            const auto resolution = screens::resolution_for_width(width);
            if (ec != std::errc{} || end != v->data() + v->size() || !resolution)
                return core::fail(core::ErrorCode::invalid_argument, std::format("unsupported resolution '{}'", *v));
            options.resolution = *resolution;
        } else {
            return core::fail(core::ErrorCode::invalid_argument, std::format("unknown argument '{}'", arg));
        }
    }
    return options;
}

}
