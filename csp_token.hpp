

#pragma once

namespace hi { inline namespace v1 {

struct csp_token {
    enum class type { verbatim, placeholder, placeholder_simple, text };

    type type;
    char const *first;
    char const *last;

    [[nodiscard]] constexpr csp_token verbatim(char const *first, char const *last) noexcept
    {
        auto r = csp_token{};
        r.type = type::verbatim;
        r.first = first;
        r.last = last;
    }

    [[nodiscard]] constexpr csp_token placeholder(char const *first, char const *last) noexcept
    {
        auto r = csp_token{};
        r.type = type::placeholder;
        r.first = first;
        r.last = last;
    }

    [[nodiscard]] constexpr csp_token placeholder_simple(char const *first, char const *last) noexcept
    {
        auto r = csp_token{};
        r.type = type::placeholder_simple;
        r.first = first;
        r.last = last;
    }

    [[nodiscard]] constexpr csp_token text(char const *first, char const *last) noexcept
    {
        auto r = csp_token{};
        r.type = type::text;
        r.first = first;
        r.last = last;
    }
};

}} // namespace hi::v1
