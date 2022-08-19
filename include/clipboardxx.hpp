#pragma once

#include "detail/interface.hpp"
#if defined(_WIN32) || defined(WIN32)
    #define WINDOWS
    #include "detail/windows.hpp"
#elif defined(__linux__)
    #define LINUX
    #include "detail/linux.hpp"
#else
    #error "platform not supported"
#endif

#include <memory>
#include <string>

namespace clipboardxx {

#ifdef WINDOWS
using clipboard_type = clipboard_windows;
#elif defined(LINUX)
using clipboard_type = clipboard_linux;
#endif

class clipboard {
public:
    clipboard() : m_clipboard(std::make_unique<clipboard_type>()) {}

    void copy(const std::string &text) const { m_clipboard->copy(text); }

    std::string paste() const { return m_clipboard->paste(); }

private:
    std::unique_ptr<clipboard_interface> m_clipboard;
};

} // namespace clipboardxx
