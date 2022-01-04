#pragma once

#include <string>
#include <exception>
#include <string.h>

#if defined(_WIN32) || defined(WIN32)
    #define WINDOWS
    #include <windows.h>
#elif defined(__linux__)
    #define LINUX
    #include <thread>
    #include <mutex>
    #include <atomic>
    #include <unistd.h>
    #include <xcb/xcb.h>
#else
    #error "platform not supported"
#endif

namespace clipboardxx {
    class exception: public std::exception {
    private:
        const char* reason;
    
    public:
        exception(const char* reason): reason(reason) { };

        const char* what() {
            return reason;
        }
    };

    class clipboard_os {
    protected:
        clipboard_os() { }

    public:
        virtual ~clipboard_os() { };
        virtual void copy(const char* text, size_t length) = 0;
        virtual void paste(std::string& dest) = 0;
    };

    #ifdef LINUX

    class clipboard_linux: public clipboard_os {
    private:
        xcb_connection_t* m_conn;
        xcb_window_t m_window;

        xcb_atom_t m_atoms[3];
        enum selection {
            clipboard = 0,
            utf8,
            result
        };

        std::thread* m_event_thread;
        std::mutex m_lock;

        class copy_data {
        public:
            char* m_data;
            size_t m_length;

            copy_data(const char* data, size_t length) {
                m_data = new char[length + 1];
                m_length = length;
                strcpy(m_data, data);
            } 

            ~copy_data() {
                delete[] m_data;
            }
        };

        copy_data* m_copy_data;
        std::atomic<xcb_get_property_reply_t*> m_paste_reply;

        static void handle_events(clipboard_linux* self) {
            xcb_generic_event_t* event;

            while(true) {
                std::lock_guard<std::mutex> guard(self->m_lock);
                event = xcb_poll_for_event(self->m_conn);

                if(event == 0) {
                    usleep(100);
                    continue;
                }

                switch(event->response_type & ~0x80) {
                    case XCB_SELECTION_NOTIFY:
                        self->m_paste_reply = xcb_get_property_reply(
                            self->m_conn, 
                            xcb_get_property(
                                self->m_conn, false, self->m_window,
                                self->m_atoms[selection::result], XCB_ATOM_ANY,
                                0, -1
                            ),
                            NULL
                        );
                        break;
                    
                    case XCB_SELECTION_CLEAR:
                        if(self->m_copy_data) {
                            delete self->m_copy_data;
                            self->m_copy_data = nullptr;
                        }
                        break;
                    
                    case XCB_SELECTION_REQUEST: {
                        if(self->m_copy_data == nullptr)
                            break;

                        auto sel_event = (xcb_selection_request_event_t*) event;
                        auto notify_event = new xcb_selection_notify_event_t {
                            .response_type = XCB_SELECTION_NOTIFY,
                            .time = XCB_CURRENT_TIME,
                            .requestor = sel_event->requestor,
                            .selection = sel_event->selection,
                            .target = sel_event->target,
                            .property = 0
                        };

                        if(sel_event->target == self->m_atoms[selection::utf8]) {
                            xcb_change_property(
                                self->m_conn, XCB_PROP_MODE_REPLACE,
                                sel_event->requestor, sel_event->property,
                                sel_event->target, 8, 
                                self->m_copy_data->m_length, self->m_copy_data->m_data
                            );

                            notify_event->property = sel_event->property;
                        }

                        xcb_send_event(self->m_conn, false, sel_event->requestor,
                            XCB_EVENT_MASK_PROPERTY_CHANGE, (char*) notify_event);
                        xcb_flush(self->m_conn);
                        delete notify_event;
                    }
                }

                free(event);
            }
        }

        clipboard_linux() {
            m_conn = xcb_connect(NULL, NULL);

            static const char* atom_names[] = {
                "CLIPBOARD",
                "UTF8_STRING",
                "RESULT"
            };

            for(int i=0; i < 3; i++) {
                const char* name = atom_names[i];
                auto reply = xcb_intern_atom_reply(
                    m_conn,
                    xcb_intern_atom(m_conn, false, strlen(name), name),
                    NULL
                );

                m_atoms[i] = reply->atom;
                free(reply);
            }

            xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(m_conn)).data;

            m_window = xcb_generate_id(m_conn);
            uint32_t mask_values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
            xcb_create_window(
                m_conn, XCB_COPY_FROM_PARENT, m_window, screen->root,
                0, 0, 1, 1, 0, 
                XCB_WINDOW_CLASS_COPY_FROM_PARENT, screen->root_visual,
                XCB_CW_EVENT_MASK, mask_values
            );

            m_paste_reply = nullptr;
            m_copy_data = nullptr;

            m_event_thread = new std::thread(handle_events, this);
            m_event_thread->detach();
        }

    public:
        virtual ~clipboard_linux() {
            delete m_event_thread;
            if(m_copy_data != nullptr)
                delete m_copy_data;

            xcb_destroy_window(m_conn, m_window);
            xcb_disconnect(m_conn);
        }

        virtual void copy(const char* text, size_t length) {
            std::lock_guard<std::mutex> guard(m_lock);

            if(m_copy_data != nullptr)
                delete m_copy_data;

            m_copy_data = new copy_data(text, length);
            xcb_set_selection_owner(m_conn, m_window, m_atoms[selection::clipboard], XCB_CURRENT_TIME);
            xcb_flush(m_conn);          
        }

        virtual void paste(std::string& dest) {
            {
                std::lock_guard<std::mutex> guard(m_lock);

                // no need to request clipboard data if we own clipboard 
                auto sel_owner = std::unique_ptr<xcb_get_selection_owner_reply_t>(
                    xcb_get_selection_owner_reply(
                        m_conn,
                        xcb_get_selection_owner(m_conn, m_atoms[selection::clipboard]),
                        NULL
                    )
                );
                
                if(sel_owner->owner == m_window) {
                    dest = m_copy_data->m_data;
                    return;
                }
                
                xcb_convert_selection(
                    m_conn, m_window,
                    m_atoms[selection::clipboard], m_atoms[selection::utf8],
                    m_atoms[selection::result], XCB_CURRENT_TIME
                );
                xcb_flush(m_conn);
            }

            while(true) {
                if(m_paste_reply != nullptr) {
                    char* value = (char*) xcb_get_property_value(m_paste_reply);
                    value[m_paste_reply.load()->value_len] = '\0';
                    
                    dest = value;
                    delete m_paste_reply.load();
                    m_paste_reply = nullptr;
                    break;
                }

                usleep(1e4);
            }
        }

        friend class clipboard;
    };

    #elif defined(WINDOWS)

    class clipboard_windows: public clipboard_os {
    private:
        clipboard_windows() {
            if(!OpenClipboard(0)) {
                throw exception("Cannot open clipboard!");
            }
        }
    
    public:
        virtual ~clipboard_windows() {
            CloseClipboard();
        }

        virtual void copy(const char* text, size_t length) {
            if(!EmptyClipboard())
                throw exception("Cannot empty clipboard!");

            HGLOBAL global = GlobalAlloc(GMEM_FIXED, (length + 1)* sizeof(char));

            #ifdef _MSC_VER
                strcpy_s((char *)global, (length + 1) * sizeof(char), text);
            #else
                strncpy((char *)global, text, length + 1);
            #endif
            
            SetClipboardData(CF_TEXT, global);
            GlobalFree(global);
        }

        virtual void paste(std::string& dest) {
            char* result = (char*) GetClipboardData(CF_TEXT);

            if(result != NULL){
                dest = result;
                GlobalFree(result);
            } else {
                dest.clear();
            }
        }

        friend class clipboard;
    };

    #endif

    class clipboard {
    private:
        clipboard_os* m_clipboard = new 
        #ifdef LINUX
            clipboard_linux
        #elif defined(WINDOWS)
            clipboard_windows
        #endif
        ;

    public:
        ~clipboard() {
            delete m_clipboard;
        }

        void operator<<(const char* text) {
            m_clipboard->copy(text, strlen(text));
        }

        void operator<<(std::string& text) {
            m_clipboard->copy(text.c_str(), text.size());
        }

        void operator>>(std::string& dest) {
            m_clipboard->paste(dest);
        }
    };
};