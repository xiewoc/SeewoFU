#include <windows.h>
#include <iostream> 
#include <fstream>
#include "Resource-user.h"
#include <shellapi.h>
#include <aclapi.h> 
#include <cstring>
#include <sstream>
#include <aclapi.h>
#include <sddl.h>
#include <string>
#include <locale>
#include <random>
#include <thread>
#include <time.h> 
#include <ctime>
#include <wchar.h>
#include <tchar.h>
#include <winternl.h>  // 包含 NtQueryInformationProcess 的定义

using namespace std;

RECT rswls;
HWND sw,swf;
NOTIFYICONDATAA nid = {};
HWND hwnd;
HANDLE hProcess = GetCurrentProcess();
BOOL logcat = 1;
BOOL logcatonmsg = 0;

typedef LONG (NTAPI *PNtQueryInformationProcess)(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength
);

// 使用 WideCharToMultiByte 转换 wstring 到 string
std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    // 计算所需缓冲区大小
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed == 0) {
        // 如果转换失败，处理错误
        DWORD error = GetLastError();
        // 这里可以添加错误处理代码
        return "";
    }

    // 创建一个足够大的 buffer
    std::string strTo(size_needed, 0);

    // 执行转换
    if (WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL) == 0) {
        // 如果转换失败，处理错误
        DWORD error = GetLastError();
        // 这里可以添加错误处理代码
        return "";
    }

    return strTo;
}

LPTSTR stringToLPTSTR(const std::string& str) {
    LPTSTR lptstr;
	// 计算所需的缓冲区大小（包括终止符）
    size_t size = str.size() + 1;
    lptstr = new TCHAR[size];
    // 复制字符串内容
    memcpy(lptstr, str.c_str(), size);
    return lptstr;
}

// 获取指定窗口句柄的程序路径
bool GetProgramPathFromHwnd(HWND hwnd, std::wstring& programPath)
{
    // 获取窗口对应的进程ID
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);

    // 打开进程以获取信息
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
    {
        return false;
    }

    // 加载 ntdll.dll 并获取 NtQueryInformationProcess 函数指针
    HMODULE hNtDll = LoadLibrary(TEXT("ntdll.dll"));
    if (hNtDll == NULL)
    {
        CloseHandle(hProcess);
        return false;
    }

    PNtQueryInformationProcess NtQueryInformationProcess = (PNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
    if (NtQueryInformationProcess == NULL)
    {
        FreeLibrary(hNtDll);
        CloseHandle(hProcess);
        return false;
    }

    // 定义 UNICOD_STRING 结构
    UNICODE_STRING uniString;
    memset(&uniString, 0, sizeof(UNICODE_STRING));
    uniString.Buffer = (PWSTR)malloc(MAX_PATH * sizeof(wchar_t));

    // 定义 PROCESS_BASIC_INFORMATION 结构
    PROCESS_BASIC_INFORMATION pbi;
    memset(&pbi, 0, sizeof(PROCESS_BASIC_INFORMATION));

    // 查询进程的基本信息
    NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    if (status >= 0)
    {
        // 查询进程的可执行文件路径
        status = NtQueryInformationProcess(hProcess, ProcessImageFileName, &uniString, sizeof(UNICODE_STRING) + (MAX_PATH - 1) * sizeof(wchar_t), NULL);
        if (status >= 0)
        {
            // 设置返回值
            programPath = std::wstring(uniString.Buffer);
        }
    }

    // 清理
    free(uniString.Buffer);
    FreeLibrary(hNtDll);
    CloseHandle(hProcess);

    return !programPath.empty();
}

// 定义SetPrivilege函数
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
        return FALSE;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
        return FALSE;

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        return FALSE;

    return TRUE;
}

BOOL TakePathFileOwnership(LPTSTR lpszOwnFile, LPTSTR generic_str)
{
    BOOL bRetval = FALSE;
    HANDLE hToken = NULL;
    PSID pSIDAdmin = NULL;
    PSID pSIDEveryone = NULL;
    PACL pACL = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    const int NUM_ACES = 2;
    EXPLICIT_ACCESS ea[NUM_ACES];
    DWORD dwRes;
    PSECURITY_DESCRIPTOR pSD;

    // 创建Everyone组的SID
    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSIDEveryone))
    {
        goto Cleanup;
    }

    // 创建BUILTIN\Administrators组的SID
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSIDAdmin))
    {
        goto Cleanup;
    }

    ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

    // 根据generic_str设置访问权限
    if (_tcsicmp(generic_str, _T("READONLY")) == 0)
    {
        ea[0].grfAccessPermissions = (FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES);
        ea[1].grfAccessPermissions = (FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES);
    }
    else if (_tcsicmp(generic_str, _T("DENYALL")) == 0)
    {
        ea[0].grfAccessPermissions = GENERIC_ALL;
        ea[0].grfAccessMode = DENY_ACCESS;
        ea[1].grfAccessPermissions = GENERIC_ALL;
        ea[1].grfAccessMode = DENY_ACCESS;
    }
    else if (_tcsicmp(generic_str, _T("GENERICALL")) == 0)
    {
        ea[0].grfAccessPermissions = GENERIC_ALL;
        ea[1].grfAccessPermissions = GENERIC_ALL;
    }

    // 设置Everyone的访问权限
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName = (LPTSTR)pSIDEveryone;

    // 设置Administrators的访问权限
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName = (LPTSTR)pSIDAdmin;

    // 设置ACL
    if (ERROR_SUCCESS != SetEntriesInAcl(NUM_ACES, ea, NULL, &pACL))
    {
        goto Cleanup;
    }

    // 获取并设置安全描述符
    if (GetNamedSecurityInfo(lpszOwnFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSD) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    // 尝试设置文件的安全信息
    dwRes = SetFileSecurity(lpszOwnFile, DACL_SECURITY_INFORMATION, pACL);
    if (dwRes == ERROR_ACCESS_DENIED)
    {
        // 如果访问被拒绝，则尝试获取所有权
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) ||
            !SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE) ||
            SetNamedSecurityInfo(lpszOwnFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, pSIDAdmin, NULL, NULL, NULL) != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        // 再次尝试设置文件的安全信息
        dwRes = SetFileSecurity(lpszOwnFile, DACL_SECURITY_INFORMATION, pACL);
    }

    if (dwRes == ERROR_SUCCESS)
    {
        bRetval = TRUE;
    }

Cleanup:
    if (pSIDAdmin) FreeSid(pSIDAdmin);
    if (pSIDEveryone) FreeSid(pSIDEveryone);
    if (pACL) LocalFree(pACL);
    if (hToken) CloseHandle(hToken);
    if (pSD) LocalFree(pSD);

    return bRetval;
}

std::string HWNDToString(HWND hwnd) {
    // 使用 stringstream 将 HWND 转换成十六进制字符串
    std::stringstream ss;
    ss << "0x" << std::hex << (uintptr_t)hwnd; // uintptr_t 用于无符号整数指针
    return ss.str();
}

std::string ReturnLog(string msg,int OOS){//写日志用的（非标准格式） PS：反之瞎写的 
	time_t timep;
	time(&timep);
	tm* p = localtime(&timep);
	const char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	string time_str = to_string(p->tm_year + 1900) + "/" + to_string(p->tm_mon + 1) + "/" + to_string(p->tm_mday) + "-" + wday[p->tm_wday] + "-" + to_string(p->tm_hour) + ":" + to_string(p->tm_min) + ":" + to_string(p->tm_sec);
	string ReLog;
	switch(OOS){
		case LOG_ERR:{
			string errstr = "[" + time_str + "]" + "[ERR] " + to_string(GetLastError()) + "detale: " + msg + "\n";
			ReLog = errstr;
			break;
		}
		case LOG_INFO:{
			string infostr = "[" + time_str + "]" + "[INFO]" + msg + "\n"; 
			ReLog = infostr;
			break;
		}
		case LOG_CATON:{
			string startstr = "[" + time_str + "]" + "[CAT] Cat is on.\n" ;
			ReLog = startstr;
			break;
		}
		case LOG_CATOFF:{
			string endstr = "[" + time_str + "]" + "[CAT] Cat is off.\n" ;
			ReLog = endstr;
			break;
		}
		default:{
			ReLog = "[" + time_str + "]" + "[SPERR]This kind of Log is under recognition.\n" ;
			break;
		}
	}
	
	return ReLog;
}

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
            	    State = 0;
            	    Sleep(300);	
				}
				else{
					SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
            	    State = 1;
            	    Sleep(300);	
				}
			}
		}
		if(State == 1){
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
		}
	Sleep(10);	
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
	
    nidb.uTimeout=10000;
	switch(MsgType)
	{
		case 1:{
			nidb.dwInfoFlags=NIIF_INFO;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 2:{
			nidb.dwInfoFlags=NIIF_WARNING;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 3:{
			nidb.dwInfoFlags=NIIF_ERROR;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		case 4:{
			nidb.dwInfoFlags=NIIF_USER;
			Shell_NotifyIcon(NIM_MODIFY, &nidb);
			break;
		}
		default:{
			nidb.dwInfoFlags=NIIF_NONE | NIIF_NOSOUND;
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

void LogCat() {
   	ofstream write_log;
   	while(1){   
   		write_log.open("./clog.log",fstream::app);//app ->追加写入 
   		if (logcatonmsg==0){
   			write_log<<ReturnLog(" ",LOG_CATON);
   			logcatonmsg = 1;
		   }
   		int cx = GetSystemMetrics(SM_CXSCREEN);   
		int cy = GetSystemMetrics(SM_CYSCREEN);
		HWND sw_log= FindWindow(NULL,"希沃管家");
		RECT rsw_log;
		if (sw!=0){
			GetClientRect(sw_log,&rsw_log);
			if(rsw_log.right==cx&&rsw_log.bottom==cy){
				string str = "Find Seewo LockScreen Window , HWND = " + HWNDToString(sw_log);
				write_log<<ReturnLog(str,LOG_INFO);
				wstring programPath;
				GetProgramPathFromHwnd(sw_log,programPath);
				string str_p = "Find Seewo LockScreen filepath , path => " + wstringToString(programPath);
				write_log<<ReturnLog(str_p,LOG_INFO);
			}
		}
		if (GetLastError()!=0&&GetLastError()!=183){//我也不知道183是为啥，反之眼不见心不烦 (183重复定义)
			string str = "?我不到啊?";
			write_log<<ReturnLog(str,LOG_ERR);
		}
		if (logcat == 0){
			write_log<<ReturnLog(" ",LOG_CATOFF);
			write_log.close();
			logcatonmsg = 0;
			break;
		}
		Sleep(1000);
    write_log.close();
	}
}

void OnTrayIcon(HWND hWnd,LPARAM lParam)
{
	POINT pt;//用于接收鼠标坐标
	MENUINFO mi;
	int menu_rtn;//用于接收菜单选项返回值
	HMENU hmenu = CreatePopupMenu();//生成菜单
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_STYLE;
	mi.dwStyle = MNS_NOCHECK | MNS_AUTODISMISS;
	HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_BG1), IMAGE_BITMAP, 110, 50, LR_CREATEDIBSECTION);
	mi.hbrBack = CreatePatternBrush(hBitmap);
	SetMenuInfo(hmenu,&mi);
	AppendMenu(hmenu, MF_STRING, IDM_LOCKAGAIN, "锁屏");
	AppendMenu(hmenu, MF_STRING, IDM_FAKESDMSG, "关机假消息");
//	AppendMenu(hmenu, MF_STRING, IDM_DESTORYBLOCKSHUTDOWN, "关闭阻止关机");
//	AppendMenu(hmenu, MF_STRING, IDM_PLAYSOUND, "放歌(?)");
	AppendMenu(hmenu, MF_SEPARATOR, 0 , NULL);
	AppendMenu(hmenu, MF_STRING, IDM_LOGCATSWICH, "开启|关闭日志");
	AppendMenu(hmenu, MF_SEPARATOR, 0 , NULL);
	AppendMenu(hmenu, MF_STRING, IDM_ABOUT, "关于");
	AppendMenu(hmenu, MF_STRING, IDM_EXIT, "退出");
	
	if (lParam == WM_RBUTTONDOWN||lParam == WM_LBUTTONDOWN)
	{
		GetCursorPos(&pt);//取鼠标坐标
		SetForegroundWindow(hWnd);
		menu_rtn = TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL );//显示菜单并获取选项ID
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
		if (menu_rtn == IDM_FAKESDMSG){
			DWORD dwFlags = EWX_FORCE | EWX_LOGOFF | EWX_FORCEIFHUNG ;
			if (BroadcastSystemMessage(EWX_FORCE | BSF_FORCEIFHUNG | BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, NULL, WM_QUERYENDSESSION, (WPARAM)TRUE, (LPARAM)dwFlags)) {
        		BroadcastSystemMessage(EWX_FORCE | BSF_FORCEIFHUNG | BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, NULL, WM_ENDSESSION, (WPARAM)TRUE, (LPARAM)dwFlags);
    		} 
		}
//		if (menu_rtn == IDM_PLAYSOUND){
//			PlaySound(MAKEINTRESOURCE(IDW_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
//			}
		if (menu_rtn == IDM_LOGCATSWICH	){
			if (logcat == 1){
				logcat = 0;
				BallonMsg(1,hWnd,"LogCat已关闭"," ");
			}
			else{
				logcat = 1;
				thread LC(LogCat);
				LC.detach();
				BallonMsg(1,hWnd,"LogCat已开启"," ");
			}
		}
		if (menu_rtn == IDM_ABOUT){
			ShellExecuteA(NULL, "open", "https://github.com/xiewoc", NULL, NULL, SW_SHOWNORMAL);
			}
		if (menu_rtn == IDM_EXIT){
		    BallonMsg(4,hWnd,"SeewoFU已退出"," ");
		    Sleep(1000);
		    logcat = 0;
		    DeleteTrayWindowIcon();
		    PostQuitMessage(0);
		}
	}
}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		
		case WM_CREATE:{
			HWND hButton = CreateWindowExW(
				0,
    			L"BUTTON",           // 预定义的按钮类名
            	L"UNLOCK1",        // 按钮文本
            	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            	18, 70, 80, 20,     // x, y, width, height
            	hwnd,                // 父窗口句柄
            	(HMENU)IDC_BUTTON,   // 控件ID
            	((LPCREATESTRUCT)lParam)->hInstance, // 应用程序实例句柄
            	NULL                 // 无附加参数
        	);
        	HWND hButton1 = CreateWindowExW(
				0,
    			L"BUTTON",           // 预定义的按钮类名
            	L"UNLOCK2",        // 按钮文本
            	WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            	18, 100, 80, 20,     // x, y, width, height
            	hwnd,                // 父窗口句柄
            	(HMENU)IDC_BUTTON1,   // 控件ID
            	((LPCREATESTRUCT)lParam)->hInstance, // 应用程序实例句柄
            	NULL                 // 无附加参数
        	);
			break;
		}
		case WM_COMMAND:{
			// 处理按钮点击
        	if (LOWORD(wParam) == IDC_BUTTON && HIWORD(wParam) == BN_CLICKED)
        	{
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
        	}
        	if (LOWORD(wParam) == IDC_BUTTON1 && HIWORD(wParam) == BN_CLICKED){
        	DWORD dwRecipients = BSM_APPLICATIONS;
			DWORD dwFlags = EWX_FORCE | EWX_LOGOFF | EWX_FORCEIFHUNG ;
				if (BroadcastSystemMessage(EWX_FORCE | BSF_FORCEIFHUNG | BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, &dwRecipients, WM_QUERYENDSESSION, (WPARAM)TRUE, (LPARAM)dwFlags)) {
        			BroadcastSystemMessage(EWX_FORCE | BSF_FORCEIFHUNG | BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK, &dwRecipients, WM_ENDSESSION, (WPARAM)TRUE, (LPARAM)dwFlags);
					TakePathFileOwnership(stringToLPTSTR("C:\\Program Files (x86)\\Seewo\\SeewoService"),stringToLPTSTR("DENYALL"));
    			} 
			}
        break;
		}
        	
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
                            rcParent.left + 10, rcParent.top + 30, 
                            0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
    	}
    	case WM_ENDSESSION:{
    		return 0;
			break;
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

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc; /* A properties struct of our window */ /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */
	
	BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	void BallonMsg(int MsgType,HWND hwnd,string INFOTITLETEXT,string INFOTEXT); 
	BOOL TakePathFileOwnership(LPTSTR lpszOwnFile, LPTSTR generic_str);
	void TrayWindowIcon(HINSTANCE hInstance,HWND hWnd,string TIPTEXT); 
	bool GetProgramPathFromHwnd(HWND hwnd, std::wstring& programPath);
	void OnTrayIcon(HWND hWnd,LPARAM lParam);
	void DeleteTrayWindowIcon();
	void ListenShow();
	void LogCat();
	
	/* zero out the struct and set the stuff we want to modify */
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL,IDC_ARROW);
	
	HBITMAP hBitmapbg = (HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_BG), IMAGE_BITMAP, 120, 192, LR_CREATEDIBSECTION);
	wc.hbrBackground = (HBRUSH)CreatePatternBrush(hBitmapbg);
	wc.lpszClassName = generateRandomString(8).c_str();
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /* Load a standard icon */
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /* use the name "A" to use the project icon */

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW ,generateRandomString(8).c_str()," ",WS_VISIBLE & ~WS_CAPTION,  //WS_EX_LAYERED
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		120, /* width */
		192, /* height */
		NULL,NULL,hInstance,NULL);
		SetWindowPos(hwnd, HWND_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOMOVE);
		
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, 0, 175, LWA_ALPHA);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
		
		TrayWindowIcon(hInstance,hwnd,"SeewoFU");
		BallonMsg(0,hwnd,"SeewoFU已成功启动","按Ctrl与Tab打开菜单");
		thread LS(ListenShow);
		LS.detach();
		thread LC(LogCat);
		LC.detach();

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
}
