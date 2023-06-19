#include <windows.h>
#include <conio.h>

int main()
{
	RECT rswls;//定义结构 
	HWND cmd =FindWindow("ConsoleWindowClass",NULL);//找cmd窗口
	SetWindowPos(cmd,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);//隐藏窗口 
		while(1)
		{
			int cx = GetSystemMetrics(SM_CXSCREEN);//获取屏幕长 
			int cy = GetSystemMetrics(SM_CYSCREEN);//获取屏幕宽
			HWND sw=FindWindow(NULL,"希沃管家");//找同名窗口 
			HWND swf =GetForegroundWindow(); //找顶置 
				if (sw!=0&&sw==swf){//顶置窗口等于找的窗口（有时需点击锁屏窗口）
					GetClientRect(sw,&rswls);//查窗口大小 
					if(rswls.right==cx&&rswls.bottom==cy){//如果大小等于屏幕大小（锁屏覆盖整块屏幕）
						SetWindowPos(sw,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);//隐藏窗口 
					}
			}
			Sleep(500);//休息0.5s（防止CPU占用过高）
		}
	return 0;
} 
