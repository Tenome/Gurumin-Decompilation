// atl_string_fakes.cpp : the Unicode string fall-backs of ATL 7 (cstringt.h), instantiated in sound3d.dll.
//
// Not game code: these are the Windows 9x replacements `GetEnvironmentVariableWFake` and `GetStringTypeExWFake`
// from <cstringt.h> (VS .NET 2002). They convert the wide arguments to the ANSI code page, call the ANSI API and
// convert the result back. The same translation unit also holds the helpers they instantiate:
//   CTempBuffer<char,128>::~CTempBuffer (0x10005490)  tail-calls FreeHeap when the text did not fit the local buffer
//   CW2AEX<128> constructor (0x100054a0) starts at the local buffer and converts with the ANSI code page
//   CTempBuffer<char,128>::AllocateBytes (0x100054d0) local buffer for <= 128 bytes, heap otherwise
// Nothing in the game calls them; the CString thunks that would are not linked.
// Needs the ATL include folder (Vc7\atlmfc\include) on the include path.
#include <atlbase.h>
#include <atlstr.h>

using namespace ATL;

// The linker only keeps the two fakes when something refers to them.
extern DWORD (WINAPI *const g_getEnvironmentVariableWFake)(LPCWSTR, LPWSTR, DWORD) = GetEnvironmentVariableWFake;
extern BOOL (WINAPI *const g_getStringTypeExWFake)(LCID, DWORD, LPCWSTR, int, LPWORD) = GetStringTypeExWFake;
