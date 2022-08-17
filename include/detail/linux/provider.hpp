#pragma once

#include <string>

namespace clipboardxx {
    class clipboard_linux_provider {
    public:
        virtual void copy(const std::string &text) = 0;
        virtual std::string paste() = 0;
        virtual ~clipboard_linux_provider() = default;
    };
}
