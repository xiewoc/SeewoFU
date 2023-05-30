#include <windows.h>
#include <conio.h>

int main()
{
	RECT rsw;//定义结构 
	HWND cmd =FindWindow("ConsoleWindowClass",NULL);
	SetWindowPos(cmd,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);//隐藏窗口 
		while(1)
		{
			HWND sw=FindWindow(NULL,"希沃管家");//找同名窗口 
			HWND swf =GetForegroundWindow(); //找顶置 
				if (sw!=0&&sw==swf){
					GetClientRect(sw,&rsw);//查窗口大小 
					if(rsw.right==1920&&rsw.bottom==1080){
						SetWindowPos(sw,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);//隐藏窗口 
					}
			}
			Sleep(1000);
		}
	return 0;
} 
