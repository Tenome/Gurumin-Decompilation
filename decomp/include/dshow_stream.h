// IDirectDrawLike, IMediaStreamLike, IDirectDrawMediaStreamLike, IAMMultiMediaStreamLike, DShowStream: declared here once (first used by src/dshow_stream.cpp).
#ifndef GURUMIN_DSHOW_STREAM_H
#define GURUMIN_DSHOW_STREAM_H

#include <windows.h>
#include "ddraw_min.h"

struct IDirectDrawLike : IUnknownLike
{
    virtual long __stdcall Compact() = 0;
    virtual long __stdcall CreateClipper() = 0;
    virtual long __stdcall CreatePalette() = 0;
    virtual long __stdcall CreateSurface(SurfaceDesc *desc, IUnknownLike **surface, IUnknownLike *outer) = 0;
};
struct IMediaStreamLike : IUnknownLike
{
};
struct IDirectDrawMediaStreamLike : IMediaStreamLike
{
    virtual long __stdcall GetMultiMediaStream() = 0;
    virtual long __stdcall GetInformation() = 0;
    virtual long __stdcall SetSameFormat() = 0;
    virtual long __stdcall AllocateSample() = 0;
    virtual long __stdcall CreateSharedSample() = 0;
    virtual long __stdcall SendEndOfStream() = 0;
    virtual long __stdcall GetFormat(SurfaceDesc *current, void **palette, SurfaceDesc *desired,
                                     unsigned long *flags) = 0;
    virtual long __stdcall SetFormat() = 0;
    virtual long __stdcall GetDirectDraw(IDirectDrawLike **directDraw) = 0;
    virtual long __stdcall SetDirectDraw() = 0;
    virtual long __stdcall CreateSample(IUnknownLike *surface, const RECT *rect, unsigned long flags,
                                        IUnknownLike **sample) = 0;
    virtual long __stdcall GetTimePerFrame(LONGLONG *time) = 0;
};
struct IAMMultiMediaStreamLike : IUnknownLike
{
    virtual long __stdcall GetInformation() = 0;
    virtual long __stdcall GetMediaStream(const GUID &purpose, IMediaStreamLike **stream) = 0;
    virtual long __stdcall EnumMediaStreams() = 0;
    virtual long __stdcall GetState() = 0;
    virtual long __stdcall SetState() = 0;
    virtual long __stdcall GetTime() = 0;
    virtual long __stdcall GetDuration() = 0;
    virtual long __stdcall Seek() = 0;
    virtual long __stdcall GetEndOfStreamEventHandle() = 0;
    virtual long __stdcall Initialize(int type, unsigned long flags, void *graph) = 0;
    virtual long __stdcall GetFilterGraph() = 0;
    virtual long __stdcall GetFilter() = 0;
    virtual long __stdcall AddMediaStream(IUnknownLike *stream, const GUID *purpose, unsigned long flags,
                                          IMediaStreamLike **added) = 0;
    virtual long __stdcall OpenFile(const WCHAR *name, unsigned long flags) = 0;
};
struct TextureObject;
struct DShowStream
{
    IAMMultiMediaStreamLike *stream;         // 0x00
    IMediaStreamLike *video;                 // 0x04: the primary video stream
    IDirectDrawMediaStreamLike *ddStream;    // 0x08
    IUnknownLike *sample;                    // 0x0c: the stream sample drawing into `surface`
    IUnknownLike *surface;                   // 0x10
    int pad_0014;
    LONGLONG frameTime;                      // 0x18: time per frame
    int width;                               // 0x20
    int height;                              // 0x24
    ~DShowStream();                          // 0x1015c440: closes the stream (a tail call of Close)
    void Close();
    long Open(const char *path);
    void M_10148910();                       // 0x10148910 (ToggleHolder::M_10148910 in texcache.cpp: same object)
    void M_10148920();                       // 0x10148920
    char M_10148930();                       // 0x10148930
    void M_10148960();                       // 0x10148960 (ToggleHolder::Poll in texcache.cpp: same object)
    void M_1015c6a0(TextureObject *texture); // 0x1015c6a0 (VideoSurface::CopyTo in surface_copy.cpp: same object)
};

#endif
