// mmio_file.h : MmioFile, the thin wrapper around a winmm (mmio) file handle that sound3d.dll's decoders read through,
// and the archive layout its opener understands.
//
// The file is either a whole file on disk (isWholeFile != 0) or a slice inside a larger archive that starts at
// baseOffset (isWholeFile == 0): then every position the callers see is relative to baseOffset.
#ifndef MMIO_FILE_H
#define MMIO_FILE_H

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

// Largest block handed to mmioRead in one call when reading a slice; the loop sleeps between blocks so the game's
// other threads keep running while a big file streams in.
const LONG MMIO_READ_BLOCK_SIZE = 0x80000;
// Milliseconds slept between two such blocks.
const DWORD MMIO_READ_BLOCK_SLEEP_MS = 2;

// Longest path (in bytes) the file opener handles.
const int MMIO_PATH_BYTES = 256;

// Length of the file name part of an archive entry (8 characters, padded with zeros) and of its extension key.
const int ARCHIVE_NAME_CHARS = 8;
const int ARCHIVE_EXTENSION_CHARS = 4;

// Marks the mmio file as a whole file (isWholeFile) as opposed to a slice of an archive.
const int FILE_IS_WHOLE = -1;

// The 8 character entry name of an archive, readable as characters and as the two words the table stores.
union ArchiveName {
    char text[ARCHIVE_NAME_CHARS];
    DWORD word[2];
};

// The 4 character extension of a file, also readable as the key word the archive's extension table stores.
union ArchiveExtension {
    char text[ARCHIVE_EXTENSION_CHARS];
    DWORD key;
};

// The lookup key of one file: the extension followed by the 8 character name. Cleared byte by byte before sscanf
// fills it.
struct ArchiveKey {
    ArchiveExtension extension;
    ArchiveName name;
};

class MmioFile {
public:
    HMMIO handle;       // mmio handle, 0 = closed
    int size;           // length of the file or archive slice in bytes (used for SEEK_END)
    int isWholeFile;    // non-zero: the file is opened on its own; zero: it is a slice starting at baseOffset
    int baseOffset;     // file position of the slice start inside the archive

    // Starts closed (expanded in place by the constructors of every object that holds a file, and for the local
    // file of 0x10006250).
    MmioFile()
    {
        handle = 0;
        isWholeFile = 0;
    }

    // Closes the handle if it is still open. Expanded in place wherever a file member is destroyed; the out-of-line
    // copy (0x100010f0) is what the unwind funclets of WaveStream::~WaveStream and 0x10006250 call.
    ~MmioFile()
    {
        if (handle) {
            mmioClose(handle, 0);
        }
        handle = 0;
    }

    // Closes the handle if open. Always returns 0. Most callers expand it in place; WaveStream::Close calls the
    // out-of-line copy 0x10001110.
    int Close()
    {
        if (handle) {
            mmioClose(handle, 0);
            handle = 0;
        }
        return 0;
    }

    // Current read position relative to the slice start, 0 when closed. Expanded in place by some callers, called
    // out of line (0x10001130) by Open and the stream openers.
    int Tell()
    {
        MMIOINFO info;

        if (!handle) {
            return 0;
        }
        mmioGetInfo(handle, &info, 0);
        if (!isWholeFile) {
            info.lBufOffset -= baseOffset;
        }
        return info.lBufOffset;
    }

    LONG Read(HPSTR buffer, int itemSize, int itemCount);   // 0x10001170, returns the last mmioRead result
    int Open(const char *path, const char *mode);           // 0x10003640, defined with the file opener

    // fseek-like: a whole file passes the request to mmioSeek; a slice maps SEEK_SET and SEEK_END onto absolute
    // archive positions. Every caller expands it in place (the out-of-line copy 0x10001210 is dead code); the
    // compiler only does so at every site with __forceinline (plain inline gave up on the later sites of the bigger
    // functions).
    __forceinline int Seek(LONG offset, int origin)
    {
        LONG target;

        if (isWholeFile) {
            return mmioSeek(handle, offset, origin);
        }
        if (origin == SEEK_SET) {
            target = baseOffset;
        } else if (origin == SEEK_CUR) {
            mmioSeek(handle, offset, SEEK_CUR);
            return -1;
        } else {
            target = baseOffset + size;
        }
        target += offset;
        mmioSeek(handle, target, SEEK_SET);
        return -1;
    }
};

#endif
