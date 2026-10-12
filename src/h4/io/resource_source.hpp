#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "h4/core/error.hpp"

namespace h4::io {

class ResourceSource {
public:
    virtual ~ResourceSource() = default;

    [[nodiscard]] virtual bool contains(std::string_view name) const = 0;

    [[nodiscard]] virtual core::Result<std::vector<std::byte>> read(std::string_view name) const = 0;

protected:
    ResourceSource()                                 = default;
    ResourceSource(const ResourceSource&)            = default;
    ResourceSource& operator=(const ResourceSource&) = default;
    ResourceSource(ResourceSource&&)                 = default;
    ResourceSource& operator=(ResourceSource&&)      = default;
};

}
