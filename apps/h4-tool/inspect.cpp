#include <cstdlib>
#include <format>
#include <fstream>
#include <print>
#include <string_view>

#include "h4/assets/asset_library.hpp"
#include "h4/core/ascii.hpp"
#include "h4/formats/animation.hpp"
#include "h4/formats/placement_guide.hpp"
#include "h4/formats/table.hpp"
#include "h4/formats/terrain_texture.hpp"
#include "h4/io/archive.hpp"
#include "h4/io/archive_chain.hpp"
#include "h4/model/game_database.hpp"
#include "tool.hpp"

namespace h4::tool {

namespace {

void print_layers(const formats::BitmapGroup& group) {
    std::println("{:<34} {:>5} {:>5} {:>5} {:>5} {:>8}  {}", "layer", "x", "y", "w", "h", "ink",
                 "kind");
    for(const formats::Layer& layer : group.layers()) {
        const core::Rect r = layer.bounds();
        std::println("{:<34} {:>5} {:>5} {:>5} {:>5} {:>8}  {}", layer.name(), r.left(), r.top(),
                     r.width(), r.height(), layer.ink(),
                     layer.is_placeholder()               ? "placeholder"
                     : formats::is_placement_guide(layer) ? "guide"
                                                          : "art");
    }
    const core::Rect bounds = group.bounds();
    std::println("{} layers, bounds ({},{}) {}x{}", group.layers().size(), bounds.left(),
                 bounds.top(), bounds.width(), bounds.height());
}

}

int dump_group(Args args) {
    if(args.size() != 2) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const assets::AssetLibrary  library{ *chain };
    const formats::BitmapGroup* group = library.group(args[1]);
    if(group == nullptr)
        return fail(core::Error{ core::ErrorCode::not_found,
                                 std::format("layers.{} missing or undecodable", args[1]) });
    print_layers(*group);
    return EXIT_SUCCESS;
}

int dump_animation(Args args) {
    if(args.size() != 2) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const assets::AssetLibrary library{ *chain };
    const formats::Animation*  animation = library.animation(args[1]);
    if(animation == nullptr)
        return fail(core::Error{ core::ErrorCode::not_found,
                                 std::format("animation.{} missing or undecodable", args[1]) });
    std::println("{} ms per frame, {} steps, anchor ({},{}) {}/{}", animation->frame_ms(),
                 animation->schedule().size(), animation->anchor_x(), animation->anchor_y(),
                 animation->anchored_x(), animation->anchored_y());
    std::print("schedule:");
    for(const std::uint32_t step : animation->schedule()) std::print(" {}", step);
    std::println("");
    print_layers(animation->group());
    return EXIT_SUCCESS;
}

int dump_terrain(Args args) {
    if(args.size() != 2) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const auto bytes = chain->read(std::format("terrain.{}.h4d", args[1]));
    if(!bytes) return fail(bytes.error());
    const auto texture = formats::TerrainTexture::parse(*bytes);
    if(!texture) return fail(texture.error());
    std::size_t blank = 0;
    for(std::size_t i = 0; i < formats::kTerrainTiles; ++i)
        if(texture->blank(i)) ++blank;
    std::println("terrain.{}: {} tiles ({} blank), {} palette entries", args[1],
                 formats::kTerrainTiles, blank, texture->palette().size());
    return EXIT_SUCCESS;
}

int dump_strings(Args args) {
    if(args.empty() || args.size() > 2) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const auto bytes = chain->read("strings.Text.h4d");
    if(!bytes) return fail(bytes.error());
    const auto table = formats::Table::parse(*bytes);
    if(!table) return fail(table.error());
    const std::string_view needle = args.size() == 2 ? args[1] : std::string_view{};
    std::size_t            shown  = 0;
    for(std::size_t r = 0; r < table->row_count(); ++r) {
        const std::string_view key  = table->cell(r, 0);
        const std::string_view text = table->cell(r, 1);
        if(!needle.empty() && !core::icontains(key, needle) && !core::icontains(text, needle))
            continue;
        std::println("{:<44} {}", key, text);
        ++shown;
    }
    std::println("{} of {} rows", shown, table->row_count());
    return EXIT_SUCCESS;
}

int list_archive(Args args) {
    if(args.size() != 1) return usage();
    const auto archive = io::Archive::open(args[0]);
    if(!archive) return fail(archive.error());
    for(const io::ArchiveEntry& e : archive->entries()) {
        if(e.is_link())
            std::println("{:>10}  {} -> {}", "link", e.name, e.link_target);
        else
            std::println("{:>10}  {}", e.uncompressed_size, e.name);
    }
    std::println("{} records", archive->entries().size());
    return EXIT_SUCCESS;
}

int extract(Args args) {
    if(args.size() != 3) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const auto bytes = chain->read(args[1]);
    if(!bytes) return fail(bytes.error());
    std::ofstream out{ std::string{ args[2] }, std::ios::binary };
    out.write(reinterpret_cast<const char*>(bytes->data()),
              static_cast<std::streamsize>(bytes->size()));
    std::println("{} bytes", bytes->size());
    return out ? EXIT_SUCCESS : EXIT_FAILURE;
}

int creatures(Args args) {
    if(args.size() != 1) return usage();
    const auto chain = io::ArchiveChain::mount_installation(args[0]);
    if(!chain) return fail(chain.error());
    const auto db = model::GameDatabase::load(*chain);
    if(!db) return fail(db.error());
    std::println("{:>3}  {:<18} {:<7} {:>2} {:>4} {:>4} {:>4} {:>6}", "id", "keyword", "align",
                 "lv", "hp", "att", "def", "gold");
    for(const model::CreatureType& c : db->creatures().all())
        std::println("{:>3}  {:<18} {:<7} {:>2} {:>4} {:>4} {:>4} {:>6}", c.id().value(),
                     c.keyword(), model::to_string(c.alignment()), c.stats().level.value(),
                     c.stats().hit_points.value(), c.stats().attack, c.stats().defense,
                     c.cost()[model::Resource::gold].value());
    std::println("{} creature types", db->creatures().size());
    return EXIT_SUCCESS;
}

}
