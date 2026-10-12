#include <lzma.h>

#include <algorithm>
#include <cstdint>
#include <format>

#include "h4/core/byte_reader.hpp"
#include "h4/io/compression.hpp"

namespace h4::io {

namespace {

class LzmaStream {
public:
    LzmaStream() noexcept                    = default;
    LzmaStream(const LzmaStream&)            = delete;
    LzmaStream& operator=(const LzmaStream&) = delete;
    ~LzmaStream() {
        lzma_end(&stream_);
    }
    [[nodiscard]] lzma_stream& get() noexcept {
        return stream_;
    }

private:
    lzma_stream stream_ = LZMA_STREAM_INIT;
};

constexpr std::size_t kPropertiesSize = 5;
constexpr std::size_t kGrowth         = 1u << 20;

[[nodiscard]] core::Result<lzma_options_lzma> lzma1_options(std::span<const std::byte> input) {
    if (input.size() < kPropertiesSize)
        return core::fail(core::ErrorCode::corrupt, "LZMA1 stream shorter than its properties");
    unsigned props = std::to_integer<unsigned>(input[0]);
    if (props >= 9 * 5 * 5)
        return core::fail(core::ErrorCode::corrupt, std::format("LZMA1 properties byte {} out of range", props));
    lzma_options_lzma options{};
    options.lc = props % 9;
    props /= 9;
    options.lp        = props % 5;
    options.pb        = props / 5;
    options.dict_size = core::le_u32(input.subspan(1).first<4>());
    return options;
}

}

core::Result<std::vector<std::byte>> decode_lzma1_raw(std::span<const std::byte> input, std::size_t limit) {
    auto options = lzma1_options(input);
    if (!options)
        return std::unexpected(options.error());
    const lzma_filter filters[] = {{LZMA_FILTER_LZMA1, &*options}, {LZMA_VLI_UNKNOWN, nullptr}};

    LzmaStream stream;
    lzma_stream& zs = stream.get();
    if (lzma_raw_decoder(&zs, filters) != LZMA_OK)
        return core::fail(core::ErrorCode::io, "LZMA1 decoder initialisation failed");

    const auto coded = input.subspan(kPropertiesSize);
    zs.next_in       = reinterpret_cast<const std::uint8_t*>(coded.data());
    zs.avail_in      = coded.size();

    std::vector<std::byte> out;
    for (;;) {
        if (zs.avail_out == 0) {
            if (out.size() >= limit)
                return core::fail(core::ErrorCode::corrupt, std::format("LZMA1 stream decodes past {} bytes", limit));
            const std::size_t used = out.size();
            out.resize(std::min(limit, used + kGrowth));
            zs.next_out  = reinterpret_cast<std::uint8_t*>(out.data() + used);
            zs.avail_out = out.size() - used;
        }
        const lzma_ret rc = lzma_code(&zs, LZMA_RUN);
        if (rc == LZMA_STREAM_END)
            break;
        if (rc != LZMA_OK)
            return core::fail(core::ErrorCode::corrupt,
                              std::format("LZMA1 decode failed (liblzma rc "
                                          "{})",
                                          static_cast<int>(rc)));
        if (zs.avail_in == 0 && zs.avail_out != 0)
            break;
    }
    out.resize(static_cast<std::size_t>(zs.total_out));
    return out;
}

}
