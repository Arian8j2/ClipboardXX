#pragma once

#include "exception.hpp"
#include "interface.hpp"

#include <memory>
#include <string>

#ifdef WINDOWS
    #include <windows.h>

namespace clipboardxx {

class ClipboardWindows : public ClipboardInterface {
public:
    void copy(const std::string &text) const override {
        OpenCloseClipboardRaii clipboard_raii;

        empty_clipboard();
        std::unique_ptr<char, WindowsPtrDeleter> buffer = allocate_memory_with_size(text.size() + 1);
        write_string_to_memory_null_terminated(text, buffer.get());
        set_clipboard_data_from_memory(buffer.get());
    }

    std::string paste() const noexcept override {
        OpenCloseClipboardRaii clipboard_raii;
        return get_clipboard_data();
    }

private:
    class OpenCloseClipboardRaii {
    public:
        OpenCloseClipboardRaii() {
            if (!OpenClipboard(0))
                throw exception("Cannot open clipboard");
        }

        ~OpenCloseClipboardRaii() { CloseClipboard(); }
    };

    class WindowsException : public exception {
    public:
        WindowsException(const std::string &reason) : exception(reason + " (" + std::to_string(GetLastError()) + ")"){};
    };

    class WindowsPtrDeleter {
    public:
        void operator()(char* ptr) { GlobalFree(ptr); }
    };

    void empty_clipboard() const {
        BOOL succeed = EmptyClipboard();
        if (!succeed)
            throw WindowsException("Cannot empty clipboard");
    }

    std::unique_ptr<char, WindowsPtrDeleter> allocate_memory_with_size(size_t size) const {
        std::unique_ptr<char, WindowsPtrDeleter> global(
            reinterpret_cast<char*>(GlobalAlloc(GMEM_FIXED, sizeof(char) * size)));
        if (!global)
            throw WindowsException("Cannot allocate memory for copying text");

        return global;
    }

    void write_string_to_memory_null_terminated(const std::string &text, char* memory) const {
        std::copy(text.begin(), text.end(), memory);
        memory[text.size()] = '\0';
    }

    void set_clipboard_data_from_memory(char* memory) const { SetClipboardData(CF_TEXT, memory); }

    std::string get_clipboard_data() const {
        char* result = reinterpret_cast<char*>(GetClipboardData(CF_TEXT));
        if (!result)
            return std::string("");
        return std::string(result);
    }
};

} // namespace clipboardxx

#endif
