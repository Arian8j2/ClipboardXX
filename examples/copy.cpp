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