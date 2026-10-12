#include "h4/io/archive_chain.hpp"

#include <array>
#include <format>
#include <system_error>
#include <unordered_set>

namespace h4::io {

namespace {

struct MountSlot {
    std::string_view primary;
    std::string_view alternate;
};

constexpr std::array<MountSlot, 11> kMountOrder{{
    {"new.h4r", {}},
    {"x2_updates.h4r", "x2 updates.h4r"},
    {"x2_override.h4r", "x2 override.h4r"},
    {"storm_updates.h4r", "storm updates.h4r"},
    {"storm_override.h4r", "storm override.h4r"},
    {"updates.h4r", {}},
    {"text.h4r", {}},
    {"music.h4r", {}},
    {"movies.h4r", {}},
    {"x2.h4r", {}},
    {"storm.h4r", {}},
}};

std::filesystem::path existing(const std::filesystem::path& dir, const MountSlot& slot) {
    std::error_code ec;
    for (const std::string_view name : {slot.primary, slot.alternate}) {
        if (name.empty())
            continue;
        auto p = dir / name;
        if (std::filesystem::is_regular_file(p, ec))
            return p;
    }
    return {};
}

}

core::Result<ArchiveChain> ArchiveChain::mount_installation(const std::filesystem::path& data_dir) {
    ArchiveChain chain;
    for (const MountSlot& slot : kMountOrder) {
        const auto path = existing(data_dir, slot);
        if (path.empty())
            continue;
        if (auto archive = Archive::open(path))
            chain.mount(std::make_unique<Archive>(std::move(*archive)));
    }
    auto base = Archive::open(data_dir / kBaseArchive);
    if (!base)
        return std::unexpected{base.error()};
    chain.mount(std::make_unique<Archive>(std::move(*base)));
    return chain;
}

void ArchiveChain::mount(std::unique_ptr<Archive> archive) {
    for (const ArchiveEntry& e : archive->entries()) {
        if (e.is_link())
            aliases_.try_emplace(e.name, e.link_target);
        else
            payloads_.try_emplace(e.name, Location{archive.get(), &e});
    }
    archives_.push_back(std::move(archive));
}

const ArchiveChain::Location* ArchiveChain::locate(std::string_view name) const {
    std::string key{name};
    core::FoldedSet seen;
    for (auto alias = aliases_.find(key); alias != aliases_.end(); alias = aliases_.find(key)) {
        if (!seen.insert(key).second)
            return nullptr;
        key = alias->second;
    }
    const auto it = payloads_.find(key);
    return it == payloads_.end() ? nullptr : &it->second;
}

bool ArchiveChain::contains(std::string_view name) const {
    return locate(name) != nullptr;
}

core::Result<std::vector<std::byte>> ArchiveChain::read(std::string_view name) const {
    const Location* loc = locate(name);
    if (loc == nullptr)
        return core::fail(core::ErrorCode::not_found, std::format("no archive holds {}", name));
    return loc->archive->read_entry(*loc->entry);
}

}
