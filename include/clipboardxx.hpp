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
    #include <X11/Xlib.h>
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
        virtual void copy(const char* text, size_t length) = 0;
        virtual void paste(std::string& dest) = 0;
    };

    #ifdef LINUX

    class clipboard_linux: public clipboard_os {
    private:
        struct copydata {
            std::thread* m_thread;
            unsigned char* m_text;
            size_t m_length;
        } m_copydata;

        Display* m_display;
        Window m_window;

        Atom m_sel;
        Atom m_utf8;

        clipboard_linux() {
            m_display = XOpenDisplay(NULL);
            
            int screen = XDefaultScreen(m_display);
            Window root = RootWindow(m_display, screen);

            m_window = XCreateSimpleWindow(m_display, root, -10, -10, 1, 1, 0, 0, 0);

            m_sel = XInternAtom(m_display, "CLIPBOARD", false);
            m_utf8 = XInternAtom(m_display, "UTF8_STRING", false);
            
            m_copydata.m_thread = nullptr;
            m_copydata.m_text = nullptr;
        }
        
        static void handle_requests(clipboard_linux* self) {
            XEvent event;
            while(true) {
                XNextEvent(self->m_display, &event);
                
                if(event.type == SelectionClear) {
                    self->free_copydata();
                    return;
                }

                if(event.type == SelectionRequest) {
                    auto sel_event = &event.xselectionrequest;
                    XSelectionEvent res_event;

                    res_event.type = SelectionNotify;
                    res_event.requestor = sel_event->requestor;
                    res_event.selection = sel_event->selection;
                    res_event.target = sel_event->target;
                    res_event.time = CurrentTime;
                    res_event.property = None;

                    if(sel_event->target == self->m_utf8) {
                        XChangeProperty(self->m_display, sel_event->requestor, sel_event->property,
                            self->m_utf8, 8, PropModeReplace, self->m_copydata.m_text, self->m_copydata.m_length);

                        res_event.property = sel_event->property;
                    }

                    XSendEvent(self->m_display, sel_event->requestor, True, NoEventMask, 
                        (XEvent*) &res_event);
                }
            }
        }

        void free_ifexist(void** ptr) {
            if(*ptr != nullptr) {
                free(*ptr);
                *ptr = nullptr;
            }
        }

        void free_copydata() {
            free_ifexist((void**) &m_copydata.m_thread);
            free_ifexist((void**) &m_copydata.m_text);
        }

    public:
        ~clipboard_linux() {
            free_copydata();
            XDestroyWindow(m_display, m_window);
            XCloseDisplay(m_display);
        }

        virtual void copy(const char* text, size_t length) {
            XSetSelectionOwner(m_display, m_sel, m_window, CurrentTime);

            free_ifexist((void**) &m_copydata.m_text);

            m_copydata.m_text = new unsigned char[length];
            m_copydata.m_length = length;
            strncpy((char*) m_copydata.m_text, text, length);

            if(m_copydata.m_thread == nullptr) {
                m_copydata.m_thread = new std::thread(handle_requests, this);
                m_copydata.m_thread->detach();
            }
        }

        virtual void paste(std::string& dest) {
            Atom res = XInternAtom(m_display, "RESULT", false);

            XConvertSelection(m_display, m_sel, m_utf8, res, m_window, CurrentTime);

            XEvent event;
            while(true) {
                XNextEvent(m_display, &event);
                if(event.type == SelectionNotify) {
                    if(event.xselection.property == None) {
                        dest.clear();
                        return;
                    }

                    Atom type_return;
                    int format_return;
                    unsigned long size, index;
                    unsigned char* text;

                    XGetWindowProperty(m_display, m_window, res, 0, -1, false, AnyPropertyType,
                        &type_return, &format_return, &index, &size, &text);

                    dest = (char*) text;

                    XFree(text);
                    XDeleteProperty(m_display, m_window, res);
                    break;
                }
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
        ~clipboard_windows() {
            CloseClipboard();
        }

        virtual void copy(const char* text, size_t length) {
            if(!EmptyClipboard())
                throw exception("Cannot empty clipboard!");

            HGLOBAL global = GlobalAlloc(GMEM_FIXED, (length + 1)* sizeof(char));

            #ifdef _MSC_VER
                strcpy_s((char *)global, (length + 1) * sizeof(char), text);
            #else
                strncpy((char *)global, text, length);
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