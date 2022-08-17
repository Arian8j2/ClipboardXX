#include "provider.hpp"
#include "xcb_event.hpp"
#include "../exception.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <thread>
#include <algorithm>

#include <xcb/xcb.h>

namespace clipboardxx {
    constexpr std::array<const char*, 7> SUPPORTED_TEXT_FORMATS = {
        "UTF8_STRING",
        "text/plain;charset=utf-8",
        "text/plain;charset=UTF-8",
        "GTK_TEXT_BUFFER_CONTENTS",
        "STRING",
        "TEXT",
        "text/plain"
    };

    class xcb : public clipboard_linux_provider {
    public:
        class exception_xcb : public exception {
        public:
            exception_xcb(const std::string &reason, int32_t error_code) :
                exception(reason + " (" + std::to_string(error_code) + ")") {};
        };

        xcb() : m_conn(create_connection()), m_window(create_window(m_conn.get())) {
            m_atoms.clipboard = create_atom("CLIPBOARD"); 
            m_atoms.buffer = create_atom("BUFFER"); 
            m_atoms.targets = create_atom("TARGETS"); 
            m_atoms.atom = create_atom("ATOM"); 

            m_atoms.supported_text_formats = std::vector<xcb_atom_t>(SUPPORTED_TEXT_FORMATS.size());
            std::transform(SUPPORTED_TEXT_FORMATS.begin(), SUPPORTED_TEXT_FORMATS.end(), m_atoms.supported_text_formats.begin(),
                    [this](const char* name) { return create_atom(std::string(name)); });

            m_event_handler = std::make_unique<event_handler_xcb>(m_conn.get(), m_window, m_atoms);
        }

        ~xcb() override {
            xcb_destroy_window(m_conn.get(), m_window);
        }

        void copy(const std::string &text) override {
            become_clipboard_owner();
            m_event_handler->set_copy_data(text);
        }

        std::string paste() override {
            return m_event_handler->get_paste_data();
        }

    private:
        class xcb_connection_deleter {
        public:
            void operator()(xcb_connection_t* conn) {
                xcb_disconnect(conn);
            }
        };

        std::unique_ptr<xcb_connection_t, xcb_connection_deleter> create_connection() const {
            std::unique_ptr<xcb_connection_t, xcb_connection_deleter> connection(xcb_connect(nullptr, nullptr));

            int32_t error = xcb_connection_has_error(connection.get());
            if (error > 0)
                throw exception_xcb("Cannot connect to X server", error);

            return connection;
        }

        xcb_window_t create_window(xcb_connection_t* conn) const {
            xcb_screen_t* screen = get_root_screen(conn);
            xcb_window_t window = xcb_generate_id(conn);

            uint32_t mask_value = XCB_EVENT_MASK_PROPERTY_CHANGE;
            xcb_void_cookie_t cookie = xcb_create_window_checked(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 1, 1, 0,
                XCB_WINDOW_CLASS_COPY_FROM_PARENT, screen->root_visual, XCB_CW_EVENT_MASK, &mask_value);
            handle_generic_error(xcb_request_check(conn, cookie), "Cannot create window");

            return window;
        }

        xcb_screen_t* get_root_screen(xcb_connection_t* conn) const {
            const xcb_setup_t* setup_info = xcb_get_setup(conn);
            xcb_screen_iterator_t screens = xcb_setup_roots_iterator(setup_info);
            return screens.data;
        }

        xcb_atom_t create_atom(std::string name) {
            xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_conn.get(), false, name.size(), name.c_str());

            xcb_generic_error_t* error = nullptr;
            std::unique_ptr<xcb_intern_atom_reply_t> reply(xcb_intern_atom_reply(m_conn.get(), cookie, &error));
            handle_generic_error(error, "Cannot create atom with name '" + name + "'");

            return reply->atom;
        }

        void handle_generic_error(xcb_generic_error_t* error, const std::string &error_msg) const {
            std::unique_ptr<xcb_generic_error_t> error_ptr(error);
            if (error_ptr)
                throw exception_xcb(error_msg, error_ptr->error_code);
        }

        // TODO: fix race condition
        void become_clipboard_owner() {
            xcb_void_cookie_t cookie = xcb_set_selection_owner_checked(m_conn.get(), m_window, m_atoms.clipboard, XCB_CURRENT_TIME);
            handle_generic_error(xcb_request_check(m_conn.get(), cookie), "Cannot become owner of clipboard selection");
            xcb_flush(m_conn.get());
        }

        const std::unique_ptr<xcb_connection_t, xcb_connection_deleter> m_conn;
        const xcb_window_t m_window;
        essential_atoms_xcb m_atoms;
        std::unique_ptr<event_handler_xcb> m_event_handler;
    };
}
