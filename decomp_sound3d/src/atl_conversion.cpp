// atl_conversion.cpp : the ATL 7 temporary-buffer helpers that sound3d.dll instantiated for file name conversion.
//
// These are not game code. They come from <atlconv.h> (CW2AEX, the wide-to-ANSI converter with a 128 byte local
// buffer) and <atlcore.h> (CTempBuffer, the buffer it is built on) of VS .NET 2002 and are compiled out of line
// because the unwind funclets and several callers call them:
//   10003560  CW2AEX<128>::~CW2AEX               frees m_psz when it is not the local buffer (unwind funclets)
//   10003590  CTempBuffer<char,128>::FreeHeap    free(m_p) (callers 10005740, 10005490, 10005530, ...)
//   100035a0  CW2AEX<128>::Init(psz, codePage)   WideCharToMultiByte into the local buffer or a heap block
//   10003610  CTempBuffer<char,128>::AllocateHeap malloc(nBytes), AtlThrow(E_OUTOFMEMORY) when it fails
// The explicit instantiations below make the compiler emit them. Needs the ATL include folder
// (Vc7\atlmfc\include) on the include path, e.g. GURUMIN_CFLAGS=/I"...\Vc7\atlmfc\include".
#include <atlbase.h>
#include <atlconv.h>

using namespace ATL;

template class ATL::CW2AEX<128>;
template class ATL::CTempBuffer<char, 128, ATL::CCRTAllocator>;
