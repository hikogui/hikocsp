

#pragma once

#include <stdexcept>

namespace csp { inline namespace v1 {

class csp_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}}
