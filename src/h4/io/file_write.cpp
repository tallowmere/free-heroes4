#include "h4/io/file_write.hpp"

#include <format>
#include <fstream>

namespace h4::io {

core::Result<void> write_file_atomically(const std::filesystem::path& path, std::string_view contents) {
    std::error_code ec;
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
        return core::fail(core::ErrorCode::io, std::format("cannot create {}: {}", path.parent_path().string(), ec.message()));
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        out.close();
        if (!out)
            return core::fail(core::ErrorCode::io, std::format("cannot write {}", temporary.string()));
    }
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        std::filesystem::remove(temporary, ec);
        return core::fail(core::ErrorCode::io, std::format("cannot replace {}", path.string()));
    }
    return {};
}

}
