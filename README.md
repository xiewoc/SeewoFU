# SeewoFU

希沃管家锁屏破解程序

使用DevCpp作为编译器，项目文件为SeewoFU.dev并包含其拓展(FCC为SeewoFU的精简版)

发行说明（1.2/1.3）：

新：

1.菜单背景（
HBITMAP hbmp = (HBITMAP)LoadImageA(NULL,"bg.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_DEFAULTSIZE|LR_CREATEDIBSECTION);）

2.解锁更精准（
int cx = GetSystemMetrics(SM_CXSCREEN);   
int cy = GetSystemMetrics(SM_CYSCREEN);）

3.解决了一些已知问题（腾讯化

未解决：

1.在某些更新版本的机子上不能解锁（有保护并且被设为了顶置一切窗口（包括了被TOPMOST的窗口）

2.没能将BITMAP放入.rc中和菜单背景大小兼容的问题（win11上要比win10大）


FCC 差不多就停更了，后面可能会再更，以后吧
