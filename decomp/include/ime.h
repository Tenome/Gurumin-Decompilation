// StateArrays, ImeContext: declared here once (first used by src/ime.cpp).
#ifndef GURUMIN_IME_H
#define GURUMIN_IME_H

#include <windows.h>


struct StateArrays
{
    char pad_0000[0x20];
    int m_0020[8];
    float m_0040[8];
    int M_1015caa0(int value, int index);
    int M_1015cac0(float value, int index);
};
struct ImeContext
{
    HWND window;
    HIMC context;
    int pad_0008;
    CANDIDATELIST *buffer;

    ImeContext();
    ~ImeContext();  // 0x1015d4f0: releases the context (a tail call of Release)
    int Release();
    int Attach(HWND target);
    int IsOpen();
    int SetOpen(BOOL open);
    int GetConversion(DWORD *conversion, DWORD *sentence);
    int SetConversion(const DWORD *conversion, const DWORD *sentence);
    LONG GetCompositionValue();
    LONG M_1015d400();
    DWORD GetSelection();
    const char *GetCandidate(int index);
    DWORD GetPageStart();
    DWORD GetCount();
    DWORD GetPageSize();
    int GetResultString(char *out);
    int GetCompositionString(char *out);
    DWORD LoadCandidates();
    int GetModeText(char *out);
};

extern "C" ImeContext DAT_10f68124;                       // the game's IME context (ime.cpp)

#endif
