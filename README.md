# ClipboardXX
Manage clipboard in **easiest** and **cross platform** way  
*Copyright (C) 2020 Arian Rezazadeh*

## Example
```C++
#include "../ClipboardXX.hpp"
#include <string>

int main(){
    CClipboardXX clipboard;

    // copy
    clipboard‌ << "text you wanna copy";

    // paste
    std::string paste_text;
    clipboard >> paste_text;
}
```
You can also do some *error handling*
```C++
try{
    CClipboardXX clipboard;
    clipboard << "";

}catch(CExceptionXX &e){
    std::cout << e.what() << std::endl;
}
```

## Setup

### Windows
Nothing special need to do, just copy ClipboardXX.hpp to your include path

### Linux
ClipboardXX use **Gtk** for managing clipboard in linux so:
1. You need to download **Gtk** dev files first
You can do this by package manager in your distro
    ```console
    apt-get install libgtkmm-3.0-dev
    ```
    or you can download and compile it manually by their [website](https://www.gtkmm.org/en/download.html)


2. Now you need to copy ClipboardXX.hpp to your include path
3. Compile it like this
    ```console
        g++ your_file.cpp `pkg-config --cflags --libs gtkmm-3.0`
    ```