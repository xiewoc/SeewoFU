#include <iostream>
#include <windows.h>
#include <conio.h> 
#include <fstream>

using namespace std;

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		
		/* Upon destruction, tell the main thread to stop */
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc; /* A properties struct of our window */
	HWND hwnd; /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */

	/* zero out the struct and set the stuff we want to modify */
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	
	/* White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it */
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

 ifstream infile("EUC.bin",ios::in);//EUC认证 
	if(!infile) {
    MessageBoxA(0,"非认证设备,限制使用本软件。","DOTS_CopyRight 2023",MB_TOPMOST|MB_SYSTEMMODAL|MB_ICONERROR);
    Sleep(100);
    PostQuitMessage(0);
    }//未发现文件
	
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"WindowClass","FCC",WS_MINIMIZE|WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		200, /* width */
		90, /* height */
		NULL,NULL,hInstance,NULL);
	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}	
	string s;//创建存储数据用变量
	while(infile>>s)//输入文件流
    if(s=="04FC711301F3C784D66955D98D399AFB"){
	}
	else{
	MessageBoxA(0,"认证错误,限制使用本软件。","DOTS_CopyRight 2023",MB_TOPMOST|MB_SYSTEMMODAL|MB_ICONERROR);
	Sleep(1000);
	PostQuitMessage(0);
    } 	
	
	/*
		This is the heart of our program where all input is processed and 
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		
		HWND sw=FindWindow(NULL,"希沃管家");
		HWND swf =GetForegroundWindow(); 
			if (sw!=0||sw==swf){
				SetWindowPos(sw,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);
			}
		Sleep(1000);
	}
	return msg.wParam;
}
