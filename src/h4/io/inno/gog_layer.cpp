#include "h4/io/inno/gog_layer.hpp"

#include <algorithm>
#include <format>
#include <ranges>

#include "h4/core/plain_text.hpp"

namespace h4::io {

namespace {

[[nodiscard]] bool is_blank(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

[[nodiscard]] bool is_name_char(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

class CallScanner {
public:
    explicit CallScanner(std::string_view text) : text_{text} {
    }

    void skip_blanks() {
        while (pos_ < text_.size() && is_blank(text_[pos_]))
            ++pos_;
    }
    [[nodiscard]] bool at_end() const noexcept {
        return pos_ == text_.size();
    }
    [[nodiscard]] char peek() const noexcept {
        return at_end() ? '\0' : text_[pos_];
    }
    bool take(char c) {
        skip_blanks();
        if (peek() != c)
            return false;
        ++pos_;
        return true;
    }
    [[nodiscard]] std::string_view name() {
        skip_blanks();
        const std::size_t start = pos_;
        while (pos_ < text_.size() && is_name_char(text_[pos_]))
            ++pos_;
        return text_.substr(start, pos_ - start);
    }
    [[nodiscard]] std::optional<std::string> quoted() {
        std::string out;
        for (++pos_; pos_ < text_.size(); ++pos_) {
            if (text_[pos_] != '\'') {
                out += text_[pos_];
                continue;
            }
            if (pos_ + 1 < text_.size() && text_[pos_ + 1] == '\'') {
                out += '\'';
                ++pos_;
                continue;
            }
            ++pos_;
            return out;
        }
        return std::nullopt;
    }
    [[nodiscard]] std::string bare() {
        const std::size_t start = pos_;
        while (pos_ < text_.size() && !is_blank(text_[pos_]) && text_[pos_] != ',' && text_[pos_] != ')')
            ++pos_;
        return std::string{text_.substr(start, pos_ - start)};
    }

private:
    std::string_view text_;
    std::size_t pos_ = 0;
};

[[nodiscard]] core::Result<std::vector<std::string>> call_arguments(CallScanner& s) {
    std::vector<std::string> arguments;
    if (s.take(')'))
        return arguments;
    for (;;) {
        s.skip_blanks();
        if (s.peek() == '\'') {
            auto quoted = s.quoted();
            if (!quoted)
                return core::fail(core::ErrorCode::corrupt, "unterminated string");
            arguments.push_back(std::move(*quoted));
        } else {
            arguments.push_back(s.bare());
            if (arguments.back().empty())
                return core::fail(core::ErrorCode::corrupt, "empty argument");
        }
        if (s.take(')'))
            return arguments;
        if (!s.take(','))
            return core::fail(core::ErrorCode::corrupt, "expected ',' or ')'");
    }
}

[[nodiscard]] core::Result<std::uint64_t> number(const std::string& text) {
    if (const auto value = core::whole_number<std::uint64_t>(text))
        return *value;
    return core::fail(core::ErrorCode::corrupt, std::format("'{}' is not a number", text));
}

[[nodiscard]] bool is_gog(const std::optional<GogCall>& call, std::string_view name) {
    return call && (call->name == name || call->name == std::string{name} + "_dependency") && call->arguments.size() == 3;
}

[[nodiscard]] core::Result<std::string> relative_destination(std::string_view raw) {
    std::string path{raw};
    std::ranges::replace(path, '\\', '/');
    const bool rooted = path.empty() || path.front() == '/' || path.find(':') != std::string::npos;
    bool escapes      = false;
    for (const auto segment : std::views::split(path, '/')) {
        const std::string_view s{segment.begin(), segment.end()};
        escapes = escapes || s.empty() || s == "." || s == "..";
    }
    if (rooted || escapes)
        return core::fail(core::ErrorCode::corrupt, std::format("destination '{}' leaves the installation", raw));
    return path;
}

[[nodiscard]] core::Result<InstallerPart> part_of(const SetupFileEntry& entry, const GogCall& after, const SetupEntries& entries) {
    if (entry.location >= entries.data.size())
        return core::fail(core::ErrorCode::corrupt, std::format("part location {} past the data entries", entry.location));
    const SetupDataEntry& data = entries.data[entry.location];
    const auto stored          = number(after.arguments[1]);
    const auto inflated        = number(after.arguments[2]);
    if (!stored)
        return std::unexpected(stored.error());
    if (!inflated)
        return std::unexpected(inflated.error());
    if (*stored != data.file_size)
        return core::fail(core::ErrorCode::corrupt,
                          std::format("part stores {} bytes, its data entry "
                                      "{}",
                                      *stored,
                                      data.file_size));
    if (data.first_slice != data.last_slice)
        return core::fail(core::ErrorCode::unsupported, "a chunk spans slices");
    return InstallerPart{
        data.first_slice, data.chunk_offset, data.chunk_size, data.file_offset, *stored, *inflated, data.compressed(), data.encrypted()
    };
}

struct OpenFile {
    InstallerFile file;
    std::size_t remaining = 0;
};

[[nodiscard]] core::Result<void>
fold_entry(const SetupFileEntry& entry, const SetupEntries& entries, std::optional<OpenFile>& open, std::vector<InstallerFile>& out) {
    auto before = parse_gog_call(entry.before_install);
    auto after  = parse_gog_call(entry.after_install);
    if (!before)
        return std::unexpected(before.error().with_context(entry.before_install));
    if (!after)
        return std::unexpected(after.error().with_context(entry.after_install));
    if (is_gog(*before, "before_install")) {
        if (open)
            return core::fail(core::ErrorCode::corrupt, open->file.destination + ": parts missing");
        auto destination = relative_destination((*before)->arguments[1]);
        auto count       = number((*before)->arguments[2]);
        if (!destination)
            return std::unexpected(destination.error());
        if (!count || *count == 0)
            return core::fail(core::ErrorCode::corrupt, *destination + ": no part count");
        open = OpenFile{InstallerFile{std::move(*destination), {}}, *count};
    }
    if (!is_gog(*after, "after_install"))
        return {};
    if (!open)
        return core::fail(core::ErrorCode::corrupt, entry.destination + ": part of no file");
    auto part = part_of(entry, **after, entries);
    if (!part)
        return std::unexpected(part.error().with_context(open->file.destination));
    open->file.parts.push_back(*part);
    if (--open->remaining == 0) {
        out.push_back(std::move(open->file));
        open.reset();
    }
    return {};
}

}

core::Result<std::optional<GogCall>> parse_gog_call(std::string_view code) {
    CallScanner s{code};
    s.skip_blanks();
    if (s.at_end())
        return std::nullopt;
    GogCall call;
    call.name = std::string{s.name()};
    if (call.name.empty() || !s.take('('))
        return core::fail(core::ErrorCode::corrupt, "not a call");
    auto arguments = call_arguments(s);
    if (!arguments)
        return std::unexpected(arguments.error());
    call.arguments = std::move(*arguments);
    s.take(';');
    s.skip_blanks();
    if (!s.at_end())
        return core::fail(core::ErrorCode::corrupt, "text after the call");
    return call;
}

core::Result<std::vector<InstallerFile>> gog_files(const SetupEntries& entries) {
    std::vector<InstallerFile> out;
    std::optional<OpenFile> open;
    for (const SetupFileEntry& entry : entries.files) {
        const auto folded = fold_entry(entry, entries, open, out);
        if (!folded)
            return std::unexpected(folded.error());
    }
    if (open)
        return core::fail(core::ErrorCode::corrupt, open->file.destination + ": parts missing");
    return out;
}

}
