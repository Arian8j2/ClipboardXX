#pragma once

#include <string>
#include <exception>
#include <string.h>

#if defined(_WIN32) || defined(WIN32)
    #define WINDOWS 1
#elif defined(__unix__)
    #define LINUX 1
#else
    #error "Not Supported OS"
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
            strncpy((char *)pGlobal, pText, Length);
            SetClipboardData(CF_TEXT, pGlobal);

            GlobalFree(pGlobal);
        }

        void PasteText(std::string &sString){
            char* pResult = (char*) GetClipboardData(CF_TEXT);

            if(pResult == NULL)
                throw CExceptionXX("Clipboard has no data to paste!");

            sString = pResult;
            GlobalFree(pResult);
        }

        friend class CClipboardXX;
};

#elif LINUX

#include <gtk/gtk.h>

class CClipboardLinux: public IClipboardOS{
    private:
        GtkClipboard *m_pClip;

        CClipboardLinux(){
            gtk_init(0, 0);
            m_pClip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        }

        static void copy_callback(GtkClipboard *pClip, const gchar *pText, gpointer pData){
            if(pData){
                gtk_clipboard_set_text(
                    pClip, 
                    (const gchar*) pData, 
                    strlen( (const char*) pData)
                );
            }

            gtk_main_quit();
        }

        static void paste_callback(GtkClipboard *pClip, const gchar *pText, gpointer pData){
            std::string* pString = (std::string*)pData;
            (*pString) = pText;

            gtk_main_quit();
        }
        
    public:
        void CopyText(const char* pText, size_t Length){
            gtk_clipboard_request_text(m_pClip, copy_callback, (gpointer)pText);
            gtk_main();
        }

        void PasteText(std::string &sString){
            gtk_clipboard_request_text(m_pClip, paste_callback, (gpointer)&sString);
            gtk_main();
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