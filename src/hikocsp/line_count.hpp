

#pragma once

#include <cstddef>

namespace hi { inline namespace v1 {

/** Determinate the line number where the @a ptr is.
 *
 * @note This function is not thread safe.
 * @param begin A pointer to the beginning of the text.
 * @param end A pointer beyond the end of the text.
 * @param ptr A pointer within the text.
 * @return Zero-based index of the line in the text where @a ptr is located.
 */
[[nodiscard]] inline size_t line_count(char const *begin, char const *end, char const *ptr) noexcept
{
    static char const *cache_ptr = nullptr;
    static size_t cache_line = 0;

    auto current = cache_ptr;
    auto line = cache_line;

    if (current < begin or current > end or current > ptr) {
        current = begin;
        line = 0;
    }

    for (; current != ptr; ++current) {
        if (*current == '\n') {
            ++line;
        }
    }

    cache_ptr = current;
    cache_line = line;
    return line;
}

}} // namespace hi::v1
