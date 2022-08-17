#pragma once

#include <mutex>
#include <memory>
#include <optional>
#include <chrono>
#include <thread>
#include <vector>
#include <cassert>
#include <algorithm>
#include <atomic>

#include <iostream>

#include <xcb/xcb.h>

namespace clipboardxx {
    constexpr uint8_t BITS_PER_BYTE = 8;
    constexpr std::chrono::duration HANDLE_EVENTS_FOR_EVER_DELAY = std::chrono::milliseconds(50);

    struct essential_atoms_xcb {
        std::vector<xcb_atom_t> supported_text_formats;
        xcb_atom_t clipboard, targets, atom, buffer;
    };

    class event_handler_xcb {
    public:
        event_handler_xcb(xcb_connection_t* conn, xcb_window_t window, const essential_atoms_xcb &atoms) :
                m_conn(conn), m_window(window), m_atoms(atoms), m_copy_data(std::nullopt), m_paste_data(std::nullopt) {
            m_targets = std::vector<xcb_atom_t>(m_atoms.supported_text_formats.size() + 1);
            m_targets[0] = m_atoms.targets;
            std::copy(m_atoms.supported_text_formats.begin(), m_atoms.supported_text_formats.end(), m_targets.begin() + 1);

            m_stop_event_thread = false;
            m_event_thread = std::thread(&event_handler_xcb::handle_events_for_ever, this); 
        }

        ~event_handler_xcb() {
            m_stop_event_thread = true;
            m_event_thread.join();
        }

        void set_copy_data(const std::string &data) {
            std::lock_guard<std::mutex> lock_guard(m_lock);
            m_copy_data = std::optional<std::string>(data);
        }

        std::string get_paste_data() {
            {
                std::lock_guard<std::mutex> lock_guard(m_lock);
                if (do_we_own_clipoard())
                    return m_copy_data.value(); 
                else
                    request_clipboard_data();
            }

            wait_for_paste_data_with_timeout(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> lock_guard(m_lock);
            std::string result = m_paste_data.value_or(std::string(""));
            m_paste_data.reset();
            return result;
        }

    private:
        bool do_we_own_clipoard() {
            return m_copy_data.has_value();
        }

        void request_clipboard_data() {
            xcb_convert_selection_checked(m_conn, m_window, m_atoms.clipboard, m_atoms.supported_text_formats.at(0), m_atoms.buffer,
                    XCB_CURRENT_TIME);
            xcb_flush(m_conn);
        }

        void wait_for_paste_data_with_timeout(std::chrono::milliseconds timeout) {
            m_paste_data.reset();
            std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();

            while (true) {
                if (std::chrono::system_clock::now() - start_time > timeout)
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(HANDLE_EVENTS_FOR_EVER_DELAY));
                std::lock_guard<std::mutex> lock_guard(m_lock);
                
                if (m_paste_data.has_value())
                    break;
            }
        }

        void handle_events_for_ever() noexcept {
            while (true) {
                if (m_stop_event_thread)
                    break;

                std::this_thread::sleep_for(HANDLE_EVENTS_FOR_EVER_DELAY);
                std::lock_guard<std::mutex> lock_guard(m_lock);
                std::unique_ptr<xcb_generic_event_t> event = pull_event(m_conn);

                if (event)
                    handle_event(std::move(event));
            }
        }
        std::unique_ptr<xcb_generic_event_t> pull_event(xcb_connection_t* conn) {
            std::unique_ptr<xcb_generic_event_t> event(xcb_poll_for_event(conn));
            // TODO: find a workaround for this
            assert(conn != nullptr);
            return event;
        }

        void handle_event(std::unique_ptr<xcb_generic_event_t> event) {
            assert(event != nullptr);

            uint8_t event_type = event->response_type & ~0x80;
            switch (event_type) {
                // someone requested clipboard data
                case XCB_SELECTION_REQUEST:
                    handle_selection_request_event(reinterpret_cast<xcb_selection_request_event_t*>(event.get()));
                    break;
                // we are no longer owner of clipboard
                case XCB_SELECTION_CLEAR:
                    m_copy_data = std::nullopt;
                    break;
                case XCB_SELECTION_NOTIFY:
                    handle_selection_notify_event(reinterpret_cast<xcb_selection_notify_event_t*>(event.get()));
                    break;
            }
        }

        void handle_selection_request_event(xcb_selection_request_event_t* event) {
            if (event->selection != m_atoms.clipboard || !m_copy_data.has_value())
                return;

            if (event->target == m_atoms.targets) {
                write_on_propery_of_window(event->property, event->requestor, m_atoms.atom, m_targets);
                notify_property_change(event->property, event->requestor, event->selection, m_atoms.atom);
                return;
            }

            std::vector<xcb_atom_t>::const_iterator iter = std::find(m_atoms.supported_text_formats.begin(),
                                                                        m_atoms.supported_text_formats.end(), event->target);
            if (iter == m_atoms.supported_text_formats.end()) {
                notify_property_change(0, event->requestor, event->selection, event->target);
                return;
            }

            write_on_propery_of_window(event->property, event->requestor, event->target, m_copy_data.value());
            notify_property_change(event->property, event->requestor, event->selection, event->target);
        }

        void handle_selection_notify_event(xcb_selection_notify_event_t* event) {
            if (event->selection != m_atoms.clipboard || m_paste_data.has_value())
                return;

            xcb_get_property_cookie_t cookie = xcb_get_property(m_conn, static_cast<uint8_t>(true), m_window, m_atoms.buffer,
                    XCB_ATOM_ANY, 0, -1);

            xcb_generic_error_t* error = nullptr;
            std::unique_ptr<xcb_get_property_reply_t> reply(xcb_get_property_reply(m_conn, cookie, &error));
            if (error != nullptr) {
                free(error);
                return;
            }

            const char* data = reinterpret_cast<const char*>(xcb_get_property_value(reply.get()));
            uint32_t length = xcb_get_property_value_length(reply.get());
            m_paste_data = std::string(data, length);
        }

        template <typename container, typename value_type = typename container::value_type>
        void write_on_propery_of_window(xcb_atom_t property, xcb_window_t window, xcb_atom_t target, const container &data) {
            xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, window, property, target,
                sizeof(value_type) * BITS_PER_BYTE, data.size(), data.data());
        }

        void notify_property_change(xcb_atom_t property, xcb_window_t window, xcb_atom_t selection, xcb_atom_t target) {
            const xcb_selection_notify_event_t event {.response_type = XCB_SELECTION_NOTIFY, .pad0 = 0, .sequence = 0,
                    .time = XCB_CURRENT_TIME, .requestor = window, .selection = selection, .target = target,
                        .property = property};

            xcb_send_event(m_conn, false, window, XCB_EVENT_MASK_PROPERTY_CHANGE, reinterpret_cast<const char*>(&event));
            xcb_flush(m_conn);
        }

        std::mutex m_lock;
        xcb_connection_t* m_conn;
        const xcb_window_t m_window;
        const essential_atoms_xcb m_atoms;
        std::vector<xcb_atom_t> m_targets;
        std::optional<std::string> m_copy_data, m_paste_data;
        std::thread m_event_thread;
        std::atomic<bool> m_stop_event_thread;
    };
}
