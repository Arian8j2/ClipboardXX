# ClipboardXX
Header only lightweight library to **copy** and **paste** text from clipboard  
*Copyright (C) 2020 Arian Rezazadeh*

## Example
```C++
#include "clipboard.hpp"
#include <string>

int main() {
    clipboardxx::clipboard clipboard;

    // copy
    clipboard << "text you wanna copy";

    // paste
    std::string paste_text;
    clipboard >> paste_text;
}
```

## Setup

### Windows
Nothing specially need to do, just copy `clipboard.hpp` under **include** folder to your include path.

### Linux
in linux based operating systems, clipboardxx requires **X11** and **pthread** to work, Link them manually or use CMake. 

### MacOS
There is currently **no support for MacOS**, Any contribute would be much appreciated.

#### CMake
By using CMake, there is no need to manually change include path or link dependencies, Just put clipboardxx folder in your project subdirectoy and use `add_subdirectory` function to create `ClipboardXX` library and then link library to your target.
```cmake
add_subdirectory(ClipboardXX)
target_link_libraries(your_target ClipboardXX)
```
## Error handling
In certain situations such as:  
- cannot open clipboard in windows
- cannot empty clipboard in windows

clipboardxx will throw an execption of type `clipboardxx::exception` you can handle it by your own way.
