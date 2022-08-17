#ifdef LINUX

#include "exception.hpp"
#include "interface.hpp"
#include "linux/xcb.hpp"

namespace clipboardxx {
    class clipboard_linux : public clipboard_interface {
    public:
        clipboard_linux() {
            m_provider = std::make_unique<xcb>();
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
        std::unique_ptr<clipboard_linux_provider> m_provider;
    };
}

#endif
