#include "h4/model/installation.hpp"

#include <string>
#include <vector>

#include "h4/core/ascii.hpp"
#include "h4/io/archive_chain.hpp"

namespace h4::model {

InstalledContent InstalledContent::from_archive_names(std::span<const std::string_view> names) {
    InstalledContent content;
    for (const std::string_view name : names) {
        if (core::iequals(name, "storm.h4r"))
            content = content.with(Expansion::gathering_storm);
        if (core::iequals(name, "x2.h4r"))
            content = content.with(Expansion::winds_of_war);
    }
    return content;
}

InstalledContent InstalledContent::detect(const io::ArchiveChain& chain) {
    std::vector<std::string> owned;
    std::vector<std::string_view> names;
    for (const auto& archive : chain.archives())
        owned.push_back(archive->path().filename().string());
    for (const std::string& name : owned)
        names.emplace_back(name);
    return from_archive_names(names);
}

}
