// mmio_file.cpp : MmioFile, sound3d.dll's mmio file wrapper (0x10001170; the inline members are in mmio_file.h).
//
// Used by the wave/mp3 readers (open, close, seek) and the decoder objects of the sound streams.
#include "mmio_file.h"

// Reads itemSize * itemCount bytes. A slice of a big archive is read in 0x80000-byte blocks with a 2 ms sleep
// between them; a whole file is read in one call. Returns what the last mmioRead returned (bytes read). Called all over the loaders (0x10003ab0, 0x10004590, ...).
LONG MmioFile::Read(HPSTR buffer, int itemSize, int itemCount)
{
    LONG remaining;
    int done;
    int blocks;

    if (isWholeFile) {
        return mmioRead(handle, buffer, itemSize * itemCount);
    }
    remaining = itemSize * itemCount;
    done = 0;
    if (remaining > MMIO_READ_BLOCK_SIZE) {
        blocks = ((unsigned int)(remaining - (MMIO_READ_BLOCK_SIZE + 1)) >> 19) + 1;
        remaining -= blocks * MMIO_READ_BLOCK_SIZE;
        do {
            mmioRead(handle, buffer + done, MMIO_READ_BLOCK_SIZE);
            done += MMIO_READ_BLOCK_SIZE;
            Sleep(MMIO_READ_BLOCK_SLEEP_MS);
            blocks--;
        } while (blocks != 0);
    }
    return mmioRead(handle, buffer + done, remaining);
}
