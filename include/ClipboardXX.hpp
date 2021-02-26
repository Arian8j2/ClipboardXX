#pragma once

#include <string>
#include <exception>
#include <string.h>

#if defined(_WIN32) || defined(WIN32)
    #define WINDOWS 1
#else
    #define LINUX 1
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
            }
        }

        friend class CClipboardXX;
};

#elif LINUX

#include <QGuiApplication>
#include <QClipboard>

// for some reason have to declare Qt App in global (idk why)
static int gs_FakeArgc = 1; 
static QGuiApplication gs_GuiApp(gs_FakeArgc, nullptr);

class CClipboardLinux: public IClipboardOS{
    private:
        QClipboard* m_pClipboard;

        CClipboardLinux(){
            m_pClipboard = gs_GuiApp.clipboard(); 
        }
        
    public:
        ~CClipboardLinux(){
            gs_GuiApp.quit();
        }

        void CopyText(const char* pText, size_t Length){
            m_pClipboard->setText(pText, QClipboard::Mode::Clipboard);
        }

        void PasteText(std::string &sString){
            sString = m_pClipboard->text().toStdString();
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