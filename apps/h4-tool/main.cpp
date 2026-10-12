#include <cstdlib>
#include <functional>
#include <map>
#include <print>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"
#include "tool.hpp"

namespace h4::tool {

int fail(const core::Error& error) {
    std::println(stderr, "h4-tool: {}", error.message());
    return EXIT_FAILURE;
}

int usage() {
    std::print(stderr,
               "usage: h4-tool archive    <file.h4r>\n"
               "       h4-tool extract    <Data dir> <entry> <out file>\n"
               "       h4-tool creatures  <Data dir>\n"
               "       h4-tool group      <Data dir> <name>\n"
               "       h4-tool animation  <Data dir> <name>\n"
               "       h4-tool strings    <Data dir> [substring]\n"
               "       h4-tool terrain    <Data dir> <name>\n"
               "       h4-tool map        <Data dir> <file.h4c|tutorial> <out.png> [x y] [width]"
               " [viewer] [alignment]\n"
               "       h4-tool screen     <Data dir> <menu|scroll|loading|message|question|chest|artifact|army|settings|adventure|help>"
               " <out.png> [800|1024|1280|<window WxH>]\n"
               "       h4-tool screenshot <Data dir> <out.png> [800|1024|1280]\n"
               "       h4-tool gog        <setup.exe> [install dir]\n"
               "       h4-tool setup      <out.png> [progress|pick|ios|download]\n");
    return 2;
}

}

int main(int argc, char** argv) {
    using namespace h4::tool;
    const std::vector<std::string_view> all(argv + 1, argv + argc);
    if(all.empty()) return usage();

    const std::map<std::string_view, std::function<int(Args)>> commands{
        { "archive", list_archive }, { "extract", extract },          { "creatures", creatures },
        { "group", dump_group },     { "animation", dump_animation }, { "strings", dump_strings },
        { "screen", screen },        { "screenshot", screenshot },    { "terrain", dump_terrain },
        { "map", dump_map },         { "gog", gog },
        { "setup", setup_screen },
    };
    const auto it = commands.find(all.front());
    if(it == commands.end()) return usage();
    return it->second(Args{ all }.subspan(1));
}
