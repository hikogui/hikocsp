
#pragma once

#include <iterator>

namespace csp { inline namespace v1 {

enum class csp_token_type { verbatim, placeholder_argument, placeholder_filter, placeholder_end, text };

template<std::random_access_iterator It>
struct csp_token {
    It first;
    It last;
    int line_nr;
    csp_token_type kind;

    ~csp_token() = default;
    constexpr csp_token(csp_token const&) noexcept = default;
    constexpr csp_token(csp_token&&) noexcept = default;
    constexpr csp_token& operator=(csp_token const&) noexcept = default;
    constexpr csp_token& operator=(csp_token&&) noexcept = default;

    constexpr csp_token() noexcept : first(), last(), line_nr(), kind() {}

    constexpr csp_token(csp_token_type kind, int line_nr, It first, It last) noexcept :
        first(first), last(last), line_nr(line_nr), kind(kind)
    {
    }

    constexpr csp_token(csp_token_type kind, int line_nr) noexcept : csp_token(kind, line_nr, {}, {}) {}

    [[nodiscard]] constexpr std::string_view text() const noexcept
    {
        return std::string_view{first, last};
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return first == last;
    }

    explicit operator bool() const noexcept
    {
        return not empty();
    }
};

}} // namespace csp::v1
