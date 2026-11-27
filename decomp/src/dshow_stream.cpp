// Movie playback through a DirectShow multimedia stream: the file's primary video stream is decoded into a
// DirectDraw surface (DShowStream::Open; the class name comes from the game's own debug messages).

#include <windows.h>
#include <string.h>

#include "ddraw_min.h"

#include "dshow_stream.h"
#include "win_vars.h"

// 0x101488b0 (DShowStream::Close). Releases the five interfaces, last to first.
void DShowStream::Close()
{
    if (surface != 0)
        surface->Release();
    surface = 0;
    if (sample != 0)
        sample->Release();
    sample = 0;
    if (ddStream != 0)
        ddStream->Release();
    ddStream = 0;
    if (video != 0)
        video->Release();
    video = 0;
    if (stream != 0)
        stream->Release();
    stream = 0;
}

// 0x1015c450 (DShowStream::Open). Opens movie file `path`: creates the multimedia stream, adds its primary video
// stream, opens the file, gets the DirectDraw media stream and its time per frame, and creates a surface in the
// stream's format with a stream sample drawing into it. Each failing step prints "failed: DShowStream::Open(n)"
// to the debugger and returns its error. Movies larger than 1024 x 512 are closed again (returns 1).
long DShowStream::Open(const char *path)
{
    WCHAR name[0x104];
    SurfaceDesc format;
    SurfaceDesc desired;
    unsigned long flags;
    IDirectDrawLike *directDraw;
    IAMMultiMediaStreamLike *created;
    Close();
    created = 0;
    long result = CoCreateInstance(DAT_101fccdc, 0, CLSCTX_INPROC_SERVER, DAT_101fccec, (void **)&created);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(1)\r\n");
        return result;
    }
    stream = created;
    result = created->Initialize(0, 0, 0);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(2)\r\n");
        return result;
    }
    result = created->AddMediaStream(0, &DAT_101fccfc, 0, 0);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(3)\r\n");
        return result;
    }
    MultiByteToWideChar(CP_ACP, 0, path, -1, name, 0x208);
    result = stream->OpenFile(name, 0);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(5)\r\n");
        return result;
    }
    result = stream->GetMediaStream(DAT_101fccfc, &video);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(6)\r\n");
        return result;
    }
    result = video->QueryInterface(DAT_101fcccc, (void **)&ddStream);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(7)\r\n");
        return result;
    }
    ddStream->GetTimePerFrame(&frameTime);
    memset(&format, 0, sizeof(format));
    memset(&desired, 0, sizeof(desired));
    format.size = sizeof(format);
    desired.size = sizeof(desired);
    result = ddStream->GetFormat(&format, 0, &desired, &flags);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(8)\r\n");
        return result;
    }
    result = ddStream->GetDirectDraw(&directDraw);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(9)\r\n");
        return result;
    }
    result = directDraw->CreateSurface(&desired, &surface, 0);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(10)\r\n");
        return result;
    }
    directDraw->Release();
    result = ddStream->CreateSample(surface, 0, 0, &sample);
    if (result < 0)
    {
        OutputDebugStringA("failed: DShowStream::Open(11)\r\n");
        return result;
    }
    width = format.width;
    height = format.height;
    // Oversized movies are closed again; the caller sees 1 for them and the stream's result otherwise.
    bool tooLarge = height > 0x200 || width > 0x400;
    if (tooLarge)
        Close();
    return tooLarge ? 1 : result;
}
