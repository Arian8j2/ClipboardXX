#pragma once

#include <string>
#include <stdexcept>

namespace clipboardxx {
    class exception : public std::runtime_error {
    public:
        exception(const std::string &reason) : std::runtime_error(reason) {};
    };
}
