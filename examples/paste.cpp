/* Linux
	apt-get install libgtkmm-3.0-dev
	g++ paste.cpp `pkg-config --cflags --libs gtkmm-3.0`
*/

/* Windows
    g++ paste.cpp
*/

#include "../ClipboardXX.hpp"

#include <string>
#include <iostream>

int main(){
	CClipboardXX clipboard;
	
    // you must use cpp string
    std::string strResult;
    clipboard >> strResult;

    std::cout << strResult << std::endl;

	return 0;
}