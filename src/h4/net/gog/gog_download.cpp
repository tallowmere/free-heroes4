#include "h4/net/gog/gog_download.hpp"

#include <format>
#include <system_error>
#include <utility>

#include "h4/net/gog/gog_api.hpp"

namespace h4::net::gog {

namespace {

[[nodiscard]] core::Result<std::string>
fetch(HttpClient& http, const std::string& url, const std::string& bearer, std::string_view what, const std::stop_token& stop) {
    auto answer = http.get(url, bearer, stop);
    if (!answer && stop.stop_requested())
        return cancelled();
    if (!answer)
        return std::unexpected{answer.error().with_context(std::format("GOG's {}", what))};
    if (answer->status != 200)
        return core::fail(core::ErrorCode::io, std::format("GOG answered {} for its {}", answer->status, what));
    return std::move(answer->body);
}

[[nodiscard]] std::uint64_t size_of(const std::filesystem::path& file) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(file, ec);
    return ec ? 0 : size;
}

[[nodiscard]] bool is_file(const std::filesystem::path& file) {
    std::error_code ec;
    return std::filesystem::is_regular_file(file, ec);
}

}

core::Result<std::filesystem::path> download_installer(HttpClient& http,
                                                       const std::string& code,
                                                       std::uint64_t product,
                                                       std::string_view language,
                                                       const std::filesystem::path& directory,
                                                       DownloadProgress& progress,
                                                       std::stop_token stop) {
    auto exchanged = http.get(token_url(code), "", stop);
    if (!exchanged && stop.stop_requested())
        return cancelled();
    if (!exchanged)
        return std::unexpected{exchanged.error().with_context("signing in to GOG")};
    auto token = parse_access_token(exchanged->status, exchanged->body);
    if (!token)
        return std::unexpected{token.error()};
    if (stop.stop_requested())
        return cancelled();

    auto owned_body = fetch(http, std::string{kOwnedUrl}, *token, "list of owned games", stop);
    if (!owned_body)
        return std::unexpected{owned_body.error()};
    auto owns = parse_owns(*owned_body, product);
    if (!owns)
        return std::unexpected{owns.error()};
    if (!*owns)
        return core::fail(core::ErrorCode::not_found, "This GOG account does not own Heroes of Might and Magic 4: Complete.");
    auto listing = fetch(http, product_url(product), "", "product listing", stop);
    if (!listing)
        return std::unexpected{listing.error()};
    auto installers = parse_installers(*listing);
    if (!installers)
        return std::unexpected{installers.error()};
    const auto installer = choose_installer(*installers, language);
    if (!installer)
        return core::fail(core::ErrorCode::not_found, "GOG lists no Windows installer to fetch");
    if (stop.stop_requested())
        return cancelled();

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec)
        return core::fail(core::ErrorCode::io, std::format("cannot create {}: {}", directory.string(), ec.message()));

    std::uint64_t total = 0;
    for (const FileLink& file : installer->files)
        total += file.size;
    progress.bytes_total = total;
    std::string exe_name;
    bool space_checked = false;
    for (std::size_t index = 0; index < installer->files.size(); ++index) {
        const FileLink& file = installer->files[index];
        auto link            = fetch(http, file.downlink, *token, "download link", stop);
        if (!link)
            return std::unexpected{link.error()};
        auto cdn = parse_downlink(*link);
        if (!cdn)
            return std::unexpected{cdn.error()};
        if (index == 0)
            exe_name = exe_name_from(*cdn);

        const std::filesystem::path target = directory / file_name(exe_name, index);
        if (is_file(target)) {
            progress.bytes_done += size_of(target);
            continue;
        }
        if (!space_checked) {
            std::uint64_t needed = 0;
            for (std::size_t rest = index; rest < installer->files.size(); ++rest)
                if (!is_file(directory / file_name(exe_name, rest)))
                    needed += installer->files[rest].size;
            const auto space = std::filesystem::space(directory, ec);
            if (!ec && space.available < needed)
                return core::fail(core::ErrorCode::io,
                                  std::format("{} MB free at {}, the "
                                              "download needs {} MB",
                                              space.available >> 20,
                                              directory.string(),
                                              needed >> 20));
            space_checked = true;
        }
        std::filesystem::path part = target;
        part += ".part";
        std::filesystem::remove(part, ec);
        if (auto fetched = http.download(*cdn, part, progress.bytes_done, stop); !fetched)
            return std::unexpected{stop.stop_requested() ? cancelled().error() : fetched.error()};
        std::filesystem::rename(part, target, ec);
        if (ec)
            return core::fail(core::ErrorCode::io, std::format("cannot rename {}: {}", part.string(), ec.message()));
    }
    return directory / exe_name;
}

}
