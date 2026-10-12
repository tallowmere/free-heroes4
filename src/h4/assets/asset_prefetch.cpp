#include <algorithm>
#include <utility>

#include "h4/assets/asset_decode.hpp"
#include "h4/assets/asset_library.hpp"
#include "h4/io/resource_source.hpp"

namespace h4::assets {

namespace {

struct Job {
    AssetKind kind;
    std::string key;
    std::string entry;
};

template <class Cache> bool held(const Cache& cache, std::string_view key) {
    return cache.find(key) != cache.end();
}

template <class T, class Cache> void take_into(Cache& cache, Decoded<T>& decoded) {
    for (auto& [key, value] : decoded)
        cache.try_emplace(std::move(key), std::move(value));
}

Prefetched decode_batch(const io::ResourceSource& source, const std::vector<Job>& jobs, bool transitions, const std::stop_token& stop) {
    Prefetched out;
    if (transitions)
        out.transitions = decode<formats::TransitionLibrary>(source, entry_name("transition", "Transitions"));
    for (const Job& job : jobs) {
        if (stop.stop_requested())
            break;
        switch (job.kind) {
        case AssetKind::group: out.groups.emplace_back(job.key, decode<formats::BitmapGroup>(source, job.entry)); break;
        case AssetKind::terrain:
            out.terrains.emplace_back(job.key, decode<formats::TerrainTexture>(source, job.entry));
            break;
        case AssetKind::object: out.objects.emplace_back(job.key, decode<formats::AdvObject>(source, job.entry)); break;
        case AssetKind::font: out.fonts.emplace_back(job.key, decode<formats::Font>(source, job.entry)); break;
        case AssetKind::transitions: break;
        }
    }
    return out;
}

}

core::Pending<Prefetched> AssetLibrary::prefetch(std::span<const AssetRequest> requests) const {
    std::vector<Job> jobs;
    bool transitions = false;
    const auto want  = [&jobs](AssetKind kind, std::string key, std::string entry) {
        if (std::ranges::none_of(jobs, [&](const Job& j) { return j.kind == kind && j.key == key; }))
            jobs.push_back({kind, std::move(key), std::move(entry)});
    };
    for (const AssetRequest& request : requests)
        switch (request.kind) {
        case AssetKind::group:
            if (!held(groups_, request.name))
                want(request.kind, request.name, entry_name("layers", request.name));
            break;
        case AssetKind::terrain:
            if (!held(terrains_, request.name))
                want(request.kind, request.name, entry_name("terrain", request.name));
            break;
        case AssetKind::object:
            if (!held(objects_, request.name))
                want(request.kind, request.name, entry_name("adv_object", request.name));
            break;
        case AssetKind::font: {
            const std::size_t pick = ladder_index(request.size);
            for (std::size_t i = 0; i < kFontLadder.size(); ++i) {
                const std::string entry{kFontLadder[i].entry};
                if ((request.size == 0 || i == pick) && !held(fonts_, entry))
                    want(request.kind, entry, entry);
            }
            break;
        }
        case AssetKind::transitions: transitions = !transitions_read_; break;
        }

    return core::Pending<Prefetched>::run([source = source_, jobs = std::move(jobs), transitions](std::stop_token stop) {
        return decode_batch(*source, jobs, transitions, stop);
    });
}

void AssetLibrary::adopt(Prefetched done) const {
    take_into<formats::BitmapGroup>(groups_, done.groups);
    take_into<formats::TerrainTexture>(terrains_, done.terrains);
    take_into<formats::AdvObject>(objects_, done.objects);
    take_into<formats::Font>(fonts_, done.fonts);
    if (done.transitions.has_value() && !transitions_read_) {
        transitions_      = std::move(*done.transitions);
        transitions_read_ = true;
    }
}

}
