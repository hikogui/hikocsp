

#pragma once

#include <vector>
#include <string_view>

namespace hi { inline namespace v1 {

struct csp_token {
    enum class type { verbatim, placeholder, text };

    type kind;
    std::string_view text;
    std::vector<std::string_view> arguments;
    std::vector<std::string_view> filters;
    
};

}} // namespace hi::v1
