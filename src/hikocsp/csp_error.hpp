// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <stdexcept>

namespace csp { inline namespace v1 {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}}
