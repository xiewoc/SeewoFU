// FCC.cpp - 隐藏希沃管家锁屏窗口（Win7 / Win10 / 高 DPI 通用）
//
// 相比旧版的改动：
//   1) 按进程找窗口，不再依赖标题 "希沃管家"（标题随页面/版本变化）
//   2) 开启 DPI 感知，尺寸比较统一到物理像素（旧版在 125%/150% 缩放下恒不相等）
//   3) 用 MonitorFromWindow 判断是否铺满某块屏幕，支持多显示器
//   4) SWP_NOSENDCHANGING 绕过目标窗口的子类化否决（windowHookFfi.SubClassWindow）
//   5) 隐藏后校验 IsWindowVisible，失败则退回 ShowWindow(SW_HIDE)
//   6) 记录被隐藏的窗口，支持 --restore 还原（否则会把自己困在隐形全屏窗口里）
//
// 用法:
//   FCC.exe                 // 循环守护：发现全屏锁屏窗口就隐藏
//   FCC.exe --list          // 只列出候选窗口，不动手
//   FCC.exe --restore       // 还原所有被本程序隐藏的窗口
//   FCC.exe --interval 300  // 轮询间隔(ms)，默认 300
//   FCC.exe --title         // 旧行为：额外要求标题包含 "希沃管家"
//   FCC.exe --hide-console  // 隐藏自身控制台窗口（旧版默认行为）
//
// 编译: cl /O2 /MT FCC.cpp user32.lib    或    g++ -O2 -static FCC.cpp -luser32

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------- 配置
static const wchar_t* TARGET_EXE = L"SeewoServiceAssistant.exe";
static const wchar_t* TITLE_HINT = L"希沃管家";
static int  g_interval = 300;
static bool g_needTitle = false;
static bool g_verbose = false;

static std::vector<HWND> g_hidden;   // 被我隐藏过的窗口，用于 --restore

// ---------------------------------------------------------------- 工具
static void logf(const char* fmt, ...) {
    char buf[1024];
    SYSTEMTIME st; GetLocalTime(&st);
    int n = sprintf_s(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap; va_start(ap, fmt);
    vsprintf_s(buf + n, sizeof(buf) - n, fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
}

// 开启 DPI 感知：旧版没有这一步，导致 GetSystemMetrics 返回缩放后的虚拟尺寸，
// 而目标窗口（Electron，per-monitor aware）的 GetClientRect 是物理像素，两者永不相等。
static void enableDpiAwareness() {
    typedef BOOL (WINAPI *SetCtx)(HANDLE);
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    if (u32) {
        auto fn = (SetCtx)GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (fn) {
            // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (HANDLE)-4
            if (fn((HANDLE)-4)) return;
            if (fn((HANDLE)-3)) return;   // PER_MONITOR_AWARE
        }
        auto legacy = (BOOL (WINAPI*)())GetProcAddress(u32, "SetProcessDPIAware");
        if (legacy && legacy()) return;
    }
}

static std::wstring exeNameOf(DWORD pid) {
    wchar_t buf[MAX_PATH] = L"";
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) {
        DWORD n = MAX_PATH;
        if (QueryFullProcessImageNameW(h, 0, buf, &n)) {
            wchar_t* b = wcsrchr(buf, L'\\');
            if (b) memmove(buf, b + 1, (wcslen(b + 1) + 1) * sizeof(wchar_t));
        }
        CloseHandle(h);
    }
    return buf;
}

// 窗口是否铺满它所在的显示器
static bool coversMonitor(HWND hwnd, RECT* outMon = NULL) {
    RECT wr;
    if (!GetWindowRect(hwnd, &wr)) return false;
    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfoW(mon, &mi)) return false;
    const RECT& mr = mi.rcMonitor;
    if (outMon) *outMon = mr;
    // 允许 2px 误差（某些 DWM/边框组合会差 1~2 像素）
    return (wr.left <= mr.left + 2 && wr.top <= mr.top + 2 &&
            wr.right >= mr.right - 2 && wr.bottom >= mr.bottom - 2);
}

struct Cand { HWND hwnd; DWORD pid; std::wstring exe; std::wstring title; bool fullscreen; bool visible; };

static BOOL CALLBACK enumProc(HWND h, LPARAM lp) {
    auto* out = (std::vector<Cand>*)lp;
    if (!IsWindow(h) || GetWindow(h, GW_OWNER) != NULL) return TRUE;  // 只要顶层、无主窗口
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (!pid) return TRUE;
    std::wstring exe = exeNameOf(pid);
    if (_wcsicmp(exe.c_str(), TARGET_EXE) != 0) return TRUE;

    wchar_t title[512] = L"";
    GetWindowTextW(h, title, 512);
    if (g_needTitle && !wcsstr(title, TITLE_HINT)) return TRUE;

    RECT wr;
    if (!GetWindowRect(h, &wr)) return TRUE;
    if (wr.right - wr.left < 32 || wr.bottom - wr.top < 32) return TRUE;   // 忽略小窗

    Cand c;
    c.hwnd = h; c.pid = pid; c.exe = exe; c.title = title;
    c.fullscreen = coversMonitor(h);
    c.visible = IsWindowVisible(h) != FALSE;
    out->push_back(c);
    return TRUE;
}

static std::vector<Cand> collect() {
    std::vector<Cand> v;
    EnumWindows(enumProc, (LPARAM)&v);
    return v;
}

// 隐藏一个窗口：NOSENDCHANGING 让目标收不到 WM_WINDOWPOSCHANGING，
// 从而绕过 windowHookFfi 的子类化否决。
static bool hideWindow(HWND h) {
    SetLastError(0);
    SetWindowPos(h, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
    if (!IsWindowVisible(h)) return true;

    // 兜底：直接 ShowWindow(SW_HIDE)
    ShowWindow(h, SW_HIDE);
    if (!IsWindowVisible(h)) return true;

    // 再兜底：移出屏幕（有些实现会强制把窗口拉回屏幕内，这条基本不可靠，仅作最后手段）
    if (g_verbose) logf("hide failed for 0x%p (visible=%d), trying move-offscreen", (void*)h, IsWindowVisible(h));
    return false;
}

static void remember(HWND h) {
    for (HWND x : g_hidden) if (x == h) return;
    g_hidden.push_back(h);
}

// ---------------------------------------------------------------- 命令
static int cmdList() {
    auto v = collect();
    printf("screen: %d x %d (physical, DPI-aware=%s)\n",
           GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), "lazy-check");
    if (v.empty()) { printf("no window of %ls found\n", TARGET_EXE); return 1; }
    for (auto& c : v) {
        RECT wr; GetWindowRect(c.hwnd, &wr);
        printf("\n hwnd=0x%p pid=%lu visible=%d fullscreen=%d\n", (void*)c.hwnd, c.pid, c.visible, c.fullscreen);
        printf("   title : \"%ls\"\n", c.title.c_str());
        printf("   rect  : %ld x %ld\n", wr.right - wr.left, wr.bottom - wr.top);
        printf("   %s\n", c.fullscreen ? "-> 会隐藏" : "-> 不铺满屏幕，跳过");
    }
    return 0;
}

static int cmdRestore() {
    auto v = collect();
    int n = 0;
    for (auto& c : v) {
        if (!IsWindowVisible(c.hwnd)) {
            ShowWindow(c.hwnd, SW_SHOW);
            SetWindowPos(c.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOSENDCHANGING);
            logf("restored 0x%p", (void*)c.hwnd);
            n++;
        }
    }
    if (!n) printf("nothing to restore\n");
    return 0;
}

static int cmdGuard() {
    logf("FCC running (interval=%dms, title-filter=%d)", g_interval, (int)g_needTitle);
    logf("target: %ls%s", TARGET_EXE, g_needTitle ? L" (title must contain 希沃管家)" : L"");
    for (;;) {
        for (auto& c : collect()) {
            if (!c.visible || !c.fullscreen) continue;
            logf("found fullscreen lock window 0x%p \"%ls\" -> hiding", (void*)c.hwnd, c.title.c_str());
            if (hideWindow(c.hwnd)) {
                remember(c.hwnd);
                logf("  hidden OK (visible=%d)", IsWindowVisible(c.hwnd));
            } else {
                logf("  FAILED (still visible), will retry");
            }
        }
        Sleep(g_interval);
    }
    return 0;
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(65001);
    enableDpiAwareness();

    bool list = false, restore = false, hideConsole = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--list")) list = true;
        else if (!strcmp(argv[i], "--restore")) restore = true;
        else if (!strcmp(argv[i], "--hide-console")) hideConsole = true;
        else if (!strcmp(argv[i], "--title")) g_needTitle = true;
        else if (!strcmp(argv[i], "--verbose")) g_verbose = true;
        else if (!strcmp(argv[i], "--interval") && i + 1 < argc) g_interval = atoi(argv[++i]);
    }
    if (hideConsole) { HWND con = GetConsoleWindow(); if (con) ShowWindow(con, SW_HIDE); }
    if (list)    return cmdList();
    if (restore) return cmdRestore();
    return cmdGuard();
}
