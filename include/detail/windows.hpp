#ifdef WINDOWS

#pragma once

#include "interface.hpp"
#include "exception.hpp"

#include <windows.h>

namespace clipboardxx {
    class clipboard_windows : public clipboard_interface {
    public:
        void copy(const std::string &text) const override {

        }

        std::string paste() const override {
            return std::string("");
        }
    };
}

#endif
