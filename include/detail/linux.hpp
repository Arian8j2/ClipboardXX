#ifdef LINUX

#pragma once

#include "exception.hpp"
#include "interface.hpp"
#include "linux/x11_provider.hpp"

namespace clipboardxx {
    class clipboard_linux : public clipboard_interface {
    public:
        clipboard_linux() {
            m_provider = std::make_unique<X11Provider>();
        }

        void copy(const std::string &text) const override {
            try {
                m_provider->copy(text);
            } catch (const exception &error) {
                throw exception("XCB Error: " + std::string(error.what()));
            }
        }

        std::string paste() const override {
            return m_provider->paste();
        }

    private:
        std::unique_ptr<LinuxClipboardProvider> m_provider;
    };
}

#endif
