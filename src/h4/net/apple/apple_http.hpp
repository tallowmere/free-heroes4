#pragma once

#include <memory>

#include "h4/net/http.hpp"

namespace h4::net::apple {

[[nodiscard]] std::unique_ptr<HttpClient> make_http_client();

}
