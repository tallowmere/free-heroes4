#include "h4/net/gog/gog_api.hpp"

#include <algorithm>
#include <format>
#include <nlohmann/json.hpp>

namespace h4::net::gog {

namespace {

using Json = nlohmann::json;

constexpr std::string_view kClientId     = "46899977096215655";
constexpr std::string_view kClientSecret = "9d85c43b1482497dbbce61f6e4aa173a433796eeae2ca8c5f6129f2dc4de46d9";
constexpr std::string_view kRedirect     = "https://embed.gog.com/on_login_success?origin=client";
constexpr std::string_view kDefaultExe   = "setup_heroes_of_might_and_magic_4.exe";
constexpr std::string_view kUnreserved   = "-._~";
constexpr std::string_view kHexDigits    = "0123456789ABCDEF";

[[nodiscard]] bool is_alnum(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

[[nodiscard]] std::string percent_encode(std::string_view text) {
    std::string out;
    for (const char c : text) {
        if (is_alnum(c) || kUnreserved.find(c) != std::string_view::npos) {
            out += c;
            continue;
        }
        const auto byte = static_cast<unsigned char>(c);
        out += '%';
        out += kHexDigits[byte >> 4];
        out += kHexDigits[byte & 0x0F];
    }
    return out;
}

[[nodiscard]] int hex_value(char c) noexcept {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

[[nodiscard]] std::string percent_decode(std::string_view text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size()) {
            const int high = hex_value(text[i + 1]);
            const int low  = hex_value(text[i + 2]);
            if (high >= 0 && low >= 0) {
                out += static_cast<char>(high * 16 + low);
                i += 2;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}

[[nodiscard]] core::Result<Json> parse(std::string_view body, std::string_view what) {
    Json json = Json::parse(body, nullptr, false);
    if (json.is_discarded())
        return core::fail(core::ErrorCode::corrupt, std::format("GOG's {} is not JSON", what));
    return json;
}

[[nodiscard]] std::string string_at(const Json& object, const char* key) {
    const auto it = object.find(key);
    return it != object.end() && it->is_string() ? it->get<std::string>() : std::string{};
}

}

std::string login_url() {
    return std::format("https://auth.gog.com/auth?client_id={}&redirect_uri={}"
                       "&response_type=code&layout=client2",
                       kClientId,
                       percent_encode(kRedirect));
}

std::optional<std::string> code_from_landing(std::string_view url) {
    if (!url.starts_with(kLandingPrefix))
        return std::nullopt;
    const auto query = url.find('?');
    if (query == std::string_view::npos)
        return std::nullopt;
    std::string_view rest = url.substr(query + 1);
    rest                  = rest.substr(0, rest.find('#'));
    while (!rest.empty()) {
        const auto end               = rest.find('&');
        const std::string_view param = rest.substr(0, end);
        if (param.starts_with("code=") && param.size() > 5)
            return percent_decode(param.substr(5));
        if (end == std::string_view::npos)
            break;
        rest = rest.substr(end + 1);
    }
    return std::nullopt;
}

std::string token_url(std::string_view code) {
    return std::format("https://auth.gog.com/token?client_id={}&client_secret={}"
                       "&grant_type=authorization_code&code={}&redirect_uri={}",
                       kClientId,
                       kClientSecret,
                       percent_encode(code),
                       percent_encode(kRedirect));
}

core::Result<std::string> parse_access_token(int status, std::string_view body) {
    auto json = parse(body, "sign-in answer");
    if (!json)
        return std::unexpected{json.error()};
    if (status != 200 || !json->is_object()) {
        const std::string why = json->is_object() ? string_at(*json, "error_description") : "";
        return core::fail(core::ErrorCode::invalid_argument,
                          std::format("GOG refused the sign-in ({}{}{})", status, why.empty() ? "" : ": ", why));
    }
    std::string token = string_at(*json, "access_token");
    if (token.empty())
        return core::fail(core::ErrorCode::corrupt, "GOG's sign-in answer carries no token");
    return token;
}

core::Result<bool> parse_owns(std::string_view body, std::uint64_t product) {
    auto json = parse(body, "list of owned games");
    if (!json)
        return std::unexpected{json.error()};
    const auto owned = json->is_object() ? json->find("owned") : json->end();
    if (owned == json->end() || !owned->is_array())
        return core::fail(core::ErrorCode::corrupt, "GOG's list of owned games has no \"owned\"");
    return std::ranges::any_of(*owned, [product](const Json& id) {
        return id.is_number_unsigned() && id.get<std::uint64_t>() == product;
    });
}

std::string product_url(std::uint64_t product) {
    return std::format("https://api.gog.com/products/{}?expand=downloads", product);
}

core::Result<std::vector<Installer>> parse_installers(std::string_view body) {
    auto json = parse(body, "product listing");
    if (!json)
        return std::unexpected{json.error()};
    const Json* installers = nullptr;
    if (json->is_object())
        if (const auto downloads = json->find("downloads"); downloads != json->end() && downloads->is_object())
            if (const auto list = downloads->find("installers"); list != downloads->end() && list->is_array())
                installers = &*list;
    if (installers == nullptr)
        return core::fail(core::ErrorCode::corrupt, "GOG's product listing has no installers");

    std::vector<Installer> out;
    for (const Json& entry : *installers) {
        if (!entry.is_object())
            continue;
        Installer installer{
            string_at(entry, "id"), string_at(entry, "os"), string_at(entry, "language"), string_at(entry, "version"), {}
        };
        if (const auto files = entry.find("files"); files != entry.end() && files->is_array())
            for (const Json& file : *files) {
                if (!file.is_object())
                    continue;
                const auto size = file.find("size");
                FileLink link{string_at(file, "id"),
                              size != file.end() && size->is_number_unsigned() ? size->get<std::uint64_t>() : 0,
                              string_at(file, "downlink")};
                if (!link.downlink.empty())
                    installer.files.push_back(std::move(link));
            }
        if (!installer.files.empty())
            out.push_back(std::move(installer));
    }
    return out;
}

std::optional<Installer> choose_installer(std::span<const Installer> installers, std::string_view language) {
    const auto windows = [](const Installer& installer) { return installer.os == "windows"; };
    const auto match   = std::ranges::find_if(installers, [&](const Installer& installer) {
        return windows(installer) && installer.language == language;
    });
    if (match != installers.end())
        return *match;
    const auto first = std::ranges::find_if(installers, windows);
    if (first != installers.end())
        return *first;
    return std::nullopt;
}

core::Result<std::string> parse_downlink(std::string_view body) {
    auto json = parse(body, "download link");
    if (!json)
        return std::unexpected{json.error()};
    std::string link = json->is_object() ? string_at(*json, "downlink") : "";
    if (link.empty())
        return core::fail(core::ErrorCode::corrupt, "GOG gave no download link");
    return link;
}

std::string exe_name_from(std::string_view cdn_url) {
    std::string_view path  = cdn_url.substr(0, cdn_url.find_first_of("?#"));
    path                   = path.substr(path.rfind('/') + 1);
    const std::string name = percent_decode(path);
    const bool safe = name.size() > 4 && name.find_first_of("/\\:") == std::string::npos && name.ends_with(".exe") &&
        name.front() != '.';
    return safe ? name : std::string{kDefaultExe};
}

std::string file_name(std::string_view exe_name, std::size_t index) {
    if (index == 0)
        return std::string{exe_name};
    const std::string_view stem = exe_name.substr(0, exe_name.rfind('.'));
    return std::format("{}-{}.bin", stem, index);
}

}
