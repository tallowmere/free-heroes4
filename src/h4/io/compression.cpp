#include "h4/io/compression.hpp"

#include <zlib.h>

#include <algorithm>
#include <format>
#include <limits>

namespace h4::io {

namespace {

class InflateStream {
public:
    InflateStream() noexcept {
        ok_ = inflateInit2(&stream_, 15 + 32) == Z_OK;
    }
    InflateStream(const InflateStream&)            = delete;
    InflateStream& operator=(const InflateStream&) = delete;
    ~InflateStream() {
        if (ok_)
            inflateEnd(&stream_);
    }
    [[nodiscard]] bool ok() const noexcept {
        return ok_;
    }
    [[nodiscard]] z_stream& get() noexcept {
        return stream_;
    }

private:
    z_stream stream_{};
    bool ok_ = false;
};

constexpr std::size_t kZlibChunkLimit = std::numeric_limits<uInt>::max();

struct Inflated {
    int rc;
    bool out_full;
    std::size_t total_out;
};

[[nodiscard]] bool within_chunk_limit(std::size_t in_size, std::size_t out_size) noexcept {
    return in_size <= kZlibChunkLimit && out_size <= kZlibChunkLimit;
}

[[nodiscard]] core::Result<Inflated> inflate_once(std::span<const std::byte> input, std::span<std::byte> out, int flush) {
    InflateStream stream;
    if (!stream.ok())
        return core::fail(core::ErrorCode::io, "zlib initialisation failed");

    z_stream& zs = stream.get();
    zs.next_in   = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.data()));
    zs.avail_in  = static_cast<uInt>(input.size());
    zs.next_out  = reinterpret_cast<Bytef*>(out.data());
    zs.avail_out = static_cast<uInt>(out.size());

    const int rc = ::inflate(&zs, flush);
    return Inflated{rc, zs.avail_out == 0, static_cast<std::size_t>(zs.total_out)};
}

}

core::Result<std::vector<std::byte>> inflate_exact(std::span<const std::byte> input, std::size_t expected_size) {
    if (expected_size == 0)
        return {};
    if (!within_chunk_limit(input.size(), expected_size))
        return core::fail(core::ErrorCode::unsupported, "payload exceeds 4 GiB");

    std::vector<std::byte> out(expected_size);
    const auto run = inflate_once(input, out, Z_FINISH);
    if (!run)
        return std::unexpected(run.error());
    if (run->rc != Z_STREAM_END)
        return core::fail(core::ErrorCode::corrupt, std::format("inflate failed (zlib rc {})", run->rc));
    if (run->total_out != expected_size)
        return core::fail(core::ErrorCode::corrupt,
                          std::format("inflated {} bytes, directory says {}", run->total_out, expected_size));
    return out;
}

core::Result<std::vector<std::byte>> inflate_prefix(std::span<const std::byte> input, std::size_t limit) {
    if (limit == 0)
        return std::vector<std::byte>{};
    if (!within_chunk_limit(input.size(), limit))
        return core::fail(core::ErrorCode::unsupported, "payload exceeds 4 GiB");

    std::vector<std::byte> out(limit);
    const auto run = inflate_once(input, out, Z_SYNC_FLUSH);
    if (!run)
        return std::unexpected(run.error());
    const bool full = run->out_full && (run->rc == Z_OK || run->rc == Z_BUF_ERROR);
    if (run->rc != Z_STREAM_END && !full)
        return core::fail(core::ErrorCode::corrupt, std::format("inflate failed (zlib rc {})", run->rc));
    out.resize(run->total_out);
    return out;
}

core::Result<std::vector<std::uint8_t>> deflate_zlib(std::span<const std::uint8_t> input) {
    if (input.size() > kZlibChunkLimit)
        return core::fail(core::ErrorCode::unsupported, "input exceeds 4 GiB");
    uLongf bound = compressBound(static_cast<uLong>(input.size()));
    std::vector<std::uint8_t> out(bound);
    if (compress2(out.data(), &bound, input.data(), static_cast<uLong>(input.size()), Z_DEFAULT_COMPRESSION) != Z_OK)
        return core::fail(core::ErrorCode::io, "deflate failed");
    out.resize(bound);
    return out;
}

std::uint32_t crc32(std::span<const std::uint8_t> bytes, std::uint32_t seed) noexcept {
    std::size_t piece = std::min<std::size_t>(bytes.size(), kZlibChunkLimit);
    uLong crc         = ::crc32(seed, bytes.data(), static_cast<uInt>(piece));
    for (bytes = bytes.subspan(piece); !bytes.empty(); bytes = bytes.subspan(piece)) {
        piece = std::min<std::size_t>(bytes.size(), kZlibChunkLimit);
        crc   = ::crc32(crc, bytes.data(), static_cast<uInt>(piece));
    }
    return static_cast<std::uint32_t>(crc);
}

}
