// apt-get install libgtkmm-3.0-dev
// g++ copy.cpp `pkg-config --cflags --libs gtkmm-3.0`

#include "../ClipboardXX.hpp"
#include <string>

int main(){
	CClipboardXX clipboard;
	
	// c string way
	const char* pText = "here we go again!";
	clipboard << pText;


	// cpp string way
	std::string strText("haha");
	clipboard << strText;

	return 0;
}