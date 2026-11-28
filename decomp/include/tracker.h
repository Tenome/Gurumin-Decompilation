// Tracker: the mouse and key tracker (app_0005.cpp).
#ifndef GURUMIN_TRACKER_H
#define GURUMIN_TRACKER_H

#include <windows.h>

struct Tracker
{
    char m_0000;
    char m_0001;
    char m_0002;
    char m_0003;
    int m_0004;
    int m_0008;
    char pad_000c[8];
    int m_0014;
    int m_0018;
    HWND window;
    int m_0020;
    int m_0024;
    int M_10001010(int p, int q, int r);   // 0x10001010 (Obj::M_10001010 in app_0001.cpp)
    int M_10001110(int *pos, char *a, char *b, char *c, int *d, char *e);   // Mouse_GetState
    int M_10001090(int x, int y);
    int M_10001050(const POINT *point, int left, int top, int width, int height);   // 0x10001050 (PointInRect in app_0001.cpp; ignores `this`)
    int M_10001440();
    int M_10001550();
    int M_10001450();
    int M_100014a0();
    int M_100014b0();   // 0x100014b0 (Obj::ReportCursor in app_0001.cpp)
};

extern "C" Tracker DAT_1020d9e0;                          // the game's mouse and key tracker

#endif
