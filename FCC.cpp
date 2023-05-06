#include <iostream>
#include <windows.h>
#include <conio.h>
using namespace std;

int main()
{
	RECT rsw;
	HWND cmd =FindWindow("ConsoleWindowClass",NULL);
	SetWindowTextA(cmd,"FCC");
	SetWindowPos(cmd,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);
		while(1)
		{
			HWND sw=FindWindow(NULL,"œ£Œ÷π‹º“");
			HWND swf =GetForegroundWindow(); 
				if (sw!=0||sw==swf){
					GetClientRect(sw,&rsw);
					if(rsw.right==1920&&rsw.bottom==1080){
						SetWindowPos(sw,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);
					}
			}
			Sleep(1000);
		}
	return 0;
}
