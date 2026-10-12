#include "h4/io/file_read.hpp"

#include <format>
#include <fstream>
#include <sstream>

namespace h4::io {

core::Result<std::string> read_text_file(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    if (!in) {
        std::error_code ec;
        const bool missing = !std::filesystem::exists(path, ec) && !ec;
        return core::fail(missing ? core::ErrorCode::not_found : core::ErrorCode::io,
                          std::format("cannot read {}", path.string()));
    }
    std::ostringstream text;
    text << in.rdbuf();
    return std::move(text).str();
}

}
