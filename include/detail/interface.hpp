#pragma once

#include <string>

namespace clipboardxx {
    class clipboard_interface {
    public:
        virtual ~clipboard_interface() = default;
        virtual void copy(const std::string &text) const = 0;
        virtual std::string paste() const = 0;
    };
}
