#include "Resource.h"
#include <windows.h>
#include <shellapi.h>
#include <cstring>
#include <string>
#include <random>
#include <thread>
#include <ctime>

using namespace std;

RECT rswls;
HWND sw,swf;
NOTIFYICONDATAA nid = {};
HWND hwnd,hWndButton;

std::string generateRandomString(int length) {
    // 初始化随机数生成器
    std::mt19937 generator(time(0)); // 使用当前时间作为种子
    std::uniform_int_distribution<int> distribution(0, 61); // 0-61对应字符集：0-9(10), A-Z(26), a-z(26)

    std::string characters("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string result;

    for (int i = 0; i < length; ++i) {
        result += characters[distribution(generator)];
    }

    return result;
}

void ListenShow()
{
	BOOL State = 0; 
	while(1){
		if(VK_CLICKED(VK_CONTROL)){
			if(VK_CLICKED(VK_TAB)){
				if(State == 1){
					SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOMOVE);
            	    SetWindowPos(hWndButton, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOMOVE);
            	    State = 0;
				}
				else{
					SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
            	    SetWindowPos(hWndButton, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW| SWP_NOMOVE);
            	    State = 1;
				}
			}
		}
		if(State == 1){
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
            	SetWindowPos(hWndButton, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW| SWP_NOMOVE);
		}
	Sleep(100);	
	}
}

void BallonMsg(int MsgType,HWND hwnd,string INFOTITLETEXT,string INFOTEXT)
{
	HINSTANCE hins = NULL;
	NOTIFYICONDATAA nidb = {};
	nidb.cbSize = sizeof(nidb);
	nidb.hWnd = hwnd;
	nidb.uFlags = NIF_MESSAGE | NIF_GUID | NIF_INFO;
	nidb.uCallbackMessage=WM_USER;
	
	size_t lenot = std::min(INFOTITLETEXT.length(), sizeof(nidb.szInfoTitle) - 1);
	#ifdef _MSC_VER // Microsoft Visual C++
	    strcpy_s(nidb.szInfoTitle, sizeof(nidb.szInfoTitle), INFOTITLETEXT.c_str());
	#else
	    strncpy(nidb.szInfoTitle, INFOTITLETEXT.c_str(), sizeof(nidb.szInfoTitle) - 1);
	    nidb.szInfoTitle[sizeof(nidb.szInfoTitle) - 1] = '\0';
	#endif
	
	
	size_t lenoi = std::min(INFOTEXT.length(), sizeof(nidb.szInfo) - 1);
	#ifdef _MSC_VER // Microsoft Visual C++
	    strcpy_s(nidb.szInfo, sizeof(nidb.szInfo), INFOTEXT.c_str());
	#else
	    strncpy(nidb.szInfo, INFOTEXT.c_str(), sizeof(nidb.szInfo) - 1);
	    nidb.szInfo[sizeof(nidb.szInfo) - 1] = '\0';
	#endif
	
    nid.uTimeout=10000;
	switch(MsgType)
	{
		case 1:{
			nid.dwInfoFlags=NIIF_INFO;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 2:{
			nid.dwInfoFlags=NIIF_WARNING;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 3:{
			nid.dwInfoFlags=NIIF_ERROR;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 4:{
			nid.dwInfoFlags=NIIF_USER;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		default:{
			nid.dwInfoFlags=NIIF_NONE | NIIF_NOSOUND;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
	} 
 } 
 
void TrayWindowIcon(HINSTANCE hInstance,HWND hWnd,string TIPTEXT)
{
	HINSTANCE hins = hInstance; 
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_GUID | NIF_INFO;
	nid.hIcon =LoadIcon(hins, MAKEINTRESOURCE(IDI_NOTIFICATIONICON)); 
	size_t len = std::min(TIPTEXT.length(), sizeof(nid.szTip) - 1);
	#ifdef _MSC_VER // Microsoft Visual C++
	    strcpy_s(nid.szTip, sizeof(nid.szTip), TIPTEXT.c_str());
	#else
	    strncpy(nid.szTip, TIPTEXT.c_str(), sizeof(nid.szTip) - 1);
	    nid.szTip[sizeof(nid.szTip) - 1] = '\0';
	#endif
	nid.uCallbackMessage=WM_USER;
	Shell_NotifyIcon(NIM_ADD, &nid);
}

void DeleteTrayWindowIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

void OnTrayIcon(HWND hWnd,LPARAM lParam)
{
	POINT pt;//用于接收鼠标坐标
	MENUINFO mi;
	int menu_rtn;//用于接收菜单选项返回值
	HMENU hmenu = CreatePopupMenu();//生成菜单
	int cx = GetSystemMetrics(SM_CXSCREEN);   
	int cy = GetSystemMetrics(SM_CYSCREEN);
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_NOCHECK | MNS_AUTODISMISS;
	mi.hbrBack = (HBRUSH)(COLOR_WINDOW+1);
	SetMenuInfo(hmenu,&mi);
	AppendMenu(hmenu, MF_STRING, IDM_ABOUT, "关于");
	AppendMenu(hmenu, MF_STRING, IDM_LOCKAGAIN, "锁屏(?)");
	AppendMenu(hmenu, MF_STRING, IDM_EXIT, "退出");
	
	if (lParam == WM_RBUTTONDOWN||lParam == WM_LBUTTONDOWN)
	{
		GetCursorPos(&pt);//取鼠标坐标
		SetForegroundWindow(hWnd);
		menu_rtn = TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL );//显示菜单并获取选项ID
		if (menu_rtn == IDM_ABOUT){
			ShellExecuteA(NULL, "open", "https://github.com/xiewoc", NULL, NULL, SW_SHOWNORMAL);
			}
		if (menu_rtn == IDM_LOCKAGAIN){
            int cx = GetSystemMetrics(SM_CXSCREEN);   
			int cy = GetSystemMetrics(SM_CYSCREEN);
			sw= FindWindow(NULL,"希沃管家");
			if (sw!=0){
				GetClientRect(sw,&rswls);
				if(rswls.right==cx&&rswls.bottom==cy){
					SetWindowPos(sw,HWND_TOP,0,0,0,0,SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
				}
			}
			BallonMsg(0,hWnd,"ok"," ");
		}
		if (menu_rtn == IDM_EXIT){
		    BallonMsg(0,hWnd,"SeewoFU已退出"," ");
		    Sleep(1000);
		    DeleteTrayWindowIcon();
		    PostQuitMessage(0);
			}
	}
}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		
		/* Upon destruction, tell the main thread to stop */
		case WM_DESTROY:{
			break;
		}
		case WM_USER:{
			OnTrayIcon(hwnd, lParam);
			break;
		}
		case WM_MOVE:
        {
            // 获取子窗口句柄
            HWND hWndChild = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (hWndChild != NULL)
            {
                // 获取父窗口的新位置
                RECT rcParent;
                GetWindowRect(hwnd, &rcParent);

                // 移动子窗口，保持相对位置不变
                SetWindowPos(hWndChild, HWND_TOPMOST, 
                            rcParent.left + 10, rcParent.top + 10, 
                            0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
    	}
//		case WM_NCCALCSIZE:{
//  			return 0;
//			break;
//		}
//		case WM_NCACTIVATE:{
//  			return TRUE;
//			break;
//		}
//		case WM_ACTIVATE:{
//			int cX = GetSystemMetrics(SM_CXSCREEN);   
//			int cY = GetSystemMetrics(SM_CYSCREEN);
//			SetWindowPos(hwnd,HWND_TOPMOST,0,0,cX,cY,SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
//			break;
//		}
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProcButton1(HWND hWndButton, UINT bmessage, WPARAM wParamb, LPARAM lParamb)
{

    switch (bmessage)
    { 
    case WM_LBUTTONDOWN: { // 左键按下
            int x = LOWORD(lParamb); // 获取鼠标x坐标
            int y = HIWORD(lParamb); // 获取鼠标y坐标
            int cx = GetSystemMetrics(SM_CXSCREEN);   
			int cy = GetSystemMetrics(SM_CYSCREEN);
			sw= FindWindow(NULL,"希沃管家");
			if (sw!=0){
				GetClientRect(sw,&rswls);
				if(rswls.right==cx&&rswls.bottom==cy){
					SetWindowPos(sw,HWND_BOTTOM,0,0,0,0,SWP_HIDEWINDOW | SWP_NOOWNERZORDER);
				}
			}
		    BallonMsg(0,hwnd,"?"," ");
            return 0;
        }
    case WM_CREATE:{
    	RECT rcParent;
        GetWindowRect(hwnd, &rcParent);

                // 移动子窗口，保持相对位置不变
        SetWindowPos(hWndButton, NULL, rcParent.left + 10, rcParent.top + 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		break;
	}
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWndButton, bmessage, wParamb, lParamb);
    }

    return 0;
}

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc; /* A properties struct of our window */ /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */

	void BallonMsg(int MsgType,HWND hwnd,string INFOTITLETEXT,string INFOTEXT); 
	void TrayWindowIcon(HINSTANCE hInstance,HWND hWnd,string TIPTEXT); 
	void OnTrayIcon(HWND hWnd,LPARAM lParam);
	void DeleteTrayWindowIcon();
	void ListenShow();
	
	/* zero out the struct and set the stuff we want to modify */
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL,IDC_ARROW);
	
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = generateRandomString(8).c_str();
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,generateRandomString(8).c_str()," ",WS_VISIBLE,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		130, /* width */
		80, /* height */
		NULL,NULL,hInstance,NULL);
		SetWindowPos(hwnd, HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, 110, 80, SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOMOVE);
		

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	
	RECT rcParent;
    GetWindowRect(hwnd, &rcParent);
	
	WNDCLASSEX wcb;
	MSG bmsg;
	
	memset(&wcb,0,sizeof(wcb));
	wcb.cbSize		 = sizeof(WNDCLASSEX);
	wcb.lpfnWndProc	 = WndProcButton1; /* This is where we will send messages to */
	wcb.hInstance	 = hInstance;
	wcb.hCursor		 = LoadCursor(NULL,IDC_ARROW);
	
	HBITMAP hBitmap = (HBITMAP)LoadImage(hInstance,MAKEINTRESOURCE(IDB_BG), IMAGE_BITMAP, 110, 50, LR_CREATEDIBSECTION);
	wcb.hbrBackground = CreatePatternBrush(hBitmap);
	wcb.lpszClassName = "Button1";
	wcb.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wcb.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	if(!RegisterClassEx(&wcb)) {
		MessageBox(NULL, "Button Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hWndButton = CreateWindowEx(WS_EX_TOOLWINDOW,"Button1",generateRandomString(16).c_str(),WS_POPUP | WS_VISIBLE | WS_CHILD,
		rcParent.left + 10, 
		rcParent.top + 10,
		110, /* width */
		50, /* height */
		hwnd,NULL,hInstance,NULL);
    SetWindowPos(hWndButton, HWND_TOPMOST, 0, 0, 0, 0,SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOMOVE);
	
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hWndButton);

	if(hWndButton == NULL) {
		MessageBox(NULL, "Button Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	
		TrayWindowIcon(hInstance,hwnd,"SeewoFU");
		BallonMsg(0,hwnd,"SeewoFU已成功启动","按Ctrl与Tab打开菜单");
		thread LS(ListenShow);
		LS.detach();
		 
	/*
		This is the heart of our program where all input is processed and 
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
	}
	return msg.wParam;
    
    while(GetMessage(&bmsg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&bmsg); /* Translate key codes to chars if present */
		DispatchMessage(&bmsg); /* Send it to WndProc */
	}
	return bmsg.wParam;
}
