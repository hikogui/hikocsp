// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iterator>

namespace csp { inline namespace v1 {

enum class csp_token_type { verbatim, placeholder_argument, placeholder_filter, placeholder_end, text };

template<std::random_access_iterator It>
struct csp_token {
    std::string text;
    int line_nr;
    csp_token_type kind;

    ~csp_token() = default;
    constexpr csp_token(csp_token const&) noexcept = default;
    constexpr csp_token(csp_token&&) noexcept = default;
    constexpr csp_token& operator=(csp_token const&) noexcept = default;
    constexpr csp_token& operator=(csp_token&&) noexcept = default;

    constexpr csp_token() noexcept : text(), line_nr(), kind() {}

    constexpr csp_token(csp_token_type kind, int line_nr, It first, It last) noexcept :
        text(first, last), line_nr(line_nr), kind(kind)
    {
    }

    constexpr csp_token(csp_token_type kind, int line_nr) noexcept : csp_token(kind, line_nr, {}, {}) {}

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return text.empty();
    }

    explicit operator bool() const noexcept
    {
        return not empty();
    }
};

}} // namespace csp::v1
