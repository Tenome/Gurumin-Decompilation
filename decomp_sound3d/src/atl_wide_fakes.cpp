// atl_wide_fakes.cpp : ATL 7 / MFC 7 library code that the MFC build of sound3d.dll instantiated (VS .NET 2002).
//
// Nothing here is game code. These are the Windows 9x fall-backs of <cstringt.h> (CompareStringW, lstrcmpiW,
// CharLowerW, CharUpperW converted through the ANSI code page), the thread-code-page helpers of <atlconv.h> they
// use and AtlThrow of <atlexcept.h> in its MFC form. The program never calls the fakes; taking their addresses
// makes the compiler emit them out of line. Addresses in sound3d.dll:
//   0x10001280 _AtlGetThreadACPFake    GetLocaleInfo based code page (Windows 9x / NT4)
//   0x10001300 _AtlGetThreadACPReal    returns CP_THREAD_ACP (Windows 2000 and later)
//   0x10001310 _AtlGetThreadACPThunk   first call: picks one of the two and installs it in g_pfnGetThreadACP
//   0x10001610 AtlThrow                MFC form: E_OUTOFMEMORY -> AfxThrowMemoryException, else AfxThrowOleException
//   0x10002070 _AtlInstallStringThunk  installs the Win9x fake or the NT function (InterlockedExchange)
//   0x100020b0 CompareStringWFake, 0x100021d0 lstrcmpiWFake, 0x100022e0 CharLowerWFake, 0x100023f0 CharUpperWFake
#include <afxwin.h>

using namespace ATL;

extern int (WINAPI *const g_compareStringWFake)(LCID, DWORD, LPCWSTR, int, LPCWSTR, int) = CompareStringWFake;
extern int (WINAPI *const g_lstrcmpiWFake)(LPCWSTR, LPCWSTR) = lstrcmpiWFake;
extern LPWSTR (WINAPI *const g_charLowerWFake)(LPWSTR) = CharLowerWFake;
extern LPWSTR (WINAPI *const g_charUpperWFake)(LPWSTR) = CharUpperWFake;
extern void (WINAPI *const g_atlThrow)(HRESULT) = AtlThrow;
