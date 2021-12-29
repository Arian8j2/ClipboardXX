#pragma once

#include <string>
#include <exception>
#include <string.h>

#if defined(_WIN32) || defined(WIN32)
    #define WINDOWS 1
#else
    #define LINUX 1
#endif

#ifdef LINUX
    #include <X11/Xlib.h>
    #include <thread>
#endif


class CExceptionXX : public std::exception{
    private:
        const char* m_pReason;

    public:
        CExceptionXX(const char* pReason) : m_pReason(pReason){};

        const char* what(){
            return m_pReason;
        }
};

class IClipboardOS{
    protected:
        // not accessable from other files :)
        IClipboardOS(){};

    public:
        virtual void CopyText(const char* pText, size_t Length){};
        virtual void PasteText(std::string &sString){};
};
class CClipboardXX;



#ifdef WINDOWS


#include <Windows.h>

class CClipboardWindows: public IClipboardOS{
    private:
        CClipboardWindows(){
            if(!OpenClipboard(0)){
                throw CExceptionXX("Cannot open clipboard!");
            }
        }

    public:
        ~CClipboardWindows(){
            CloseClipboard();
        }

        void CopyText(const char* pText, size_t Length){
            if(!EmptyClipboard())
                throw CExceptionXX("Cannot empty clipboard!");

            HGLOBAL pGlobal = GlobalAlloc(GMEM_FIXED, (Length + 1)* sizeof(char));

            #ifdef _MSC_VER
                strcpy_s((char *)pGlobal, (Length + 1)*sizeof(char), pText);
            #else
                strncpy((char *)pGlobal, pText, Length);
            #endif
            
            SetClipboardData(CF_TEXT, pGlobal);

            GlobalFree(pGlobal);
        }

        void PasteText(std::string &sString){
            char* pResult = (char*) GetClipboardData(CF_TEXT);

            if(pResult != NULL){
                sString = pResult;
                GlobalFree(pResult);
            } else {
                sString.clear();
            }
        }

        friend class CClipboardXX;
};

#elif LINUX

class CClipboardLinux: public IClipboardOS{
    private:
        struct CopyData {
            std::thread* m_thread;
            unsigned char* m_text;
            size_t m_length;
        } m_copydata;

        Display* m_display;
        Window m_window;

        Atom m_sel;
        Atom m_utf8;

        CClipboardLinux(){
            m_display = XOpenDisplay(NULL);
            
            int screen = XDefaultScreen(m_display);
            Window root = RootWindow(m_display, screen);

            m_window = XCreateSimpleWindow(m_display, root, -10, -10, 1, 1, 0, 0, 0);

            m_sel = XInternAtom(m_display, "CLIPBOARD", false);
            m_utf8 = XInternAtom(m_display, "UTF8_STRING", false);
            
            m_copydata.m_thread = nullptr;
            m_copydata.m_text = nullptr;
        }
        
        static void handle_requests(CClipboardLinux* self) {
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
        ~CClipboardLinux(){
            free_copydata();
            XDestroyWindow(m_display, m_window);
            XCloseDisplay(m_display);
        }

        void CopyText(const char* text, size_t len){
            XSetSelectionOwner(m_display, m_sel, m_window, CurrentTime);

            free_ifexist((void**) &m_copydata.m_text);

            m_copydata.m_text = new unsigned char[len];
            m_copydata.m_length = len;
            strncpy((char*) m_copydata.m_text, text, len);

            if(m_copydata.m_thread == nullptr) {
                m_copydata.m_thread = new std::thread(handle_requests, this);
                m_copydata.m_thread->detach();
            }
        }

        void PasteText(std::string &sString){
            Atom res = XInternAtom(m_display, "RESULT", false);

            XConvertSelection(m_display, m_sel, m_utf8, res, m_window, CurrentTime);

            XEvent event;
            while(true) {
                XNextEvent(m_display, &event);
                if(event.type == SelectionNotify) {
                    if(event.xselection.property == None) {
                        sString.clear();
                        return;
                    }

                    Atom type_return;
                    int format_return;
                    unsigned long size, index;
                    unsigned char* text;

                    XGetWindowProperty(m_display, m_window, res, 0, -1, false, AnyPropertyType,
                        &type_return, &format_return, &index, &size, &text);

                    sString = (char*) text;

                    XFree(text);
                    XDeleteProperty(m_display, m_window, res);
                    break;
                }
            }
        }

        friend class CClipboardXX;
};

#endif

class CClipboardXX{
    private:
        IClipboardOS* m_pClipboard = new 
        
        #ifdef WINDOWS
            CClipboardWindows();
        #elif LINUX
            CClipboardLinux();
        #endif


    public:
        ~CClipboardXX(){
            delete m_pClipboard;
        }       
        void operator<<(const char* pText){
            m_pClipboard->CopyText(pText, strlen(pText));
        }

        void operator<<(std::string &sText){
            m_pClipboard->CopyText(sText.c_str(), sText.size());
        }

        void operator>>(std::string &sResult){
            m_pClipboard->PasteText(sResult);
        }
};