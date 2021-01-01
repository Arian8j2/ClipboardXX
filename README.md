# ClipboardXX
Manage clipboard in **easiest** and **cross platform** way  
*Copyright (C) 2020 Arian Rezazadeh*

## Example
```C++
#include "ClipboardXX.hpp"
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
If some error happens like cannot opening clipboard and etc. You can use  _**error handling**_
```C++
try{

    CClipboardXX clipboard;
    clipboard << "something";

}catch(CExceptionXX &e){

    std::cout << e.what() << std::endl;

}
```

## Setup

### Windows
Nothing specially need to do, just copy `include/ClipboardXX.hpp` to your include path or you can use CMake

### Unix
ClipboardXX in Unix based operating systems is just a wrapper around **Qt5 Clipboard**   
so you can use CMake for compilation and just use:
```cmake
    add_subdirectory(ClipboardXX)
```
in your cmake file. include directories and links will be automatically setted 
