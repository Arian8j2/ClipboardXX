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