#pragma once

#include <optional>
#include <span>
#include <string_view>

namespace h4::core {
class Error;
struct Size;
}
namespace h4::assets {
class AssetLibrary;
}
namespace h4::graphics {
class Canvas;
}
namespace h4::io {
class ArchiveChain;
}

namespace h4::tool {

using Args = std::span<const std::string_view>;

int fail(const core::Error& error);
int usage();

int list_archive(Args args);
int extract(Args args);
int creatures(Args args);
int dump_group(Args args);
int dump_animation(Args args);
int dump_strings(Args args);
int dump_terrain(Args args);

int gog(Args args);
int setup_screen(Args args);

int dump_map(Args args);
int screen(Args args);
int screenshot(Args args);
[[nodiscard]] std::optional<int> render_dialog(std::string_view which,
                                               const io::ArchiveChain& chain,
                                               const assets::AssetLibrary& library,
                                               core::Size size,
                                               const graphics::Canvas& canvas);

}
