// pack_file.cpp : PackFile, the stdio wrapper BMPLoad and BMPSave use for their image files.
#include <string.h>
#include "pack_file.h"

const char PATH_SEPARATOR = 0x5c;   // backslash

// Keeps the base name of a path: the part after the last backslash, cut at the last dot (or kept whole when the
// name has no dot). Clears the first 8 bytes of the output first. Always returns 0. (PackFile::Open)
int ExtractBaseName(char *baseName, const char *path)
{
    char work[256];
    int nameStart;
    int dotIndex;
    int i;

    strcpy(work, path);
    i = nameStart = dotIndex = 0;
    memset(baseName, 0, 8);
    for (i = 0; work[i] != '\0'; i++) {
        if (work[i] == PATH_SEPARATOR)
            nameStart = i + 1;
        if (work[i] == '.')
            dotIndex = i;
    }
    if (dotIndex == 0)
        dotIndex = strlen(work);
    work[dotIndex] = '\0';
    strcpy(baseName, work + nameStart);
    return 0;
}

// Starts closed. (BMPLoad, BMPSave)
PackFile::PackFile()
{
    m_fp = 0;
}

// Closes the stream if it is still open. (BMPLoad, BMPSave, their unwind funclets)
// Same bytes as Close; the linker folds the two into one function.
PackFile::~PackFile()
{
    if (m_fp != 0)
        fclose(m_fp);
    m_fp = 0;
}

// Closes the stream if it is still open. (BMPLoad, BMPSave)
void PackFile::Close()
{
    if (m_fp != 0)
        fclose(m_fp);
    m_fp = 0;
}

// Opens a file with the given fopen mode and remembers its base name. Returns 1 on success, 0 when the path is
// empty or the file cannot be opened. (BMPLoad opens "rb", BMPSave "wb")
int PackFile::Open(const char *path, const char *mode)
{
    if (path == 0)
        return 0;
    if (*path == '\0')
        return 0;
    if (m_fp != 0)
        fclose(m_fp);
    m_fp = 0;
    ExtractBaseName(m_name, path);
    m_fp = fopen(path, mode);
    return m_fp != 0;
}

// Reads size * count bytes in blocks of PACK_FILE_READ_BLOCK. Returns the result of the last fread, 0 when closed.
// (BMPLoad)
size_t PackFile::Read(void *buffer, size_t size, size_t count)
{
    int total;
    int done;

    if (m_fp != 0) {
        total = size * count;
        done = 0;
        while (total > PACK_FILE_READ_BLOCK) {
            fread((char *)buffer + done, 1, PACK_FILE_READ_BLOCK, m_fp);
            total -= PACK_FILE_READ_BLOCK;
            done += PACK_FILE_READ_BLOCK;
        }
        return fread((char *)buffer + done, 1, total, m_fp);
    }
    return 0;
}

// Writes size * count bytes; returns what fwrite returned, 0 when closed. (BMPSave)
size_t PackFile::Write(const void *buffer, size_t size, size_t count)
{
    if (m_fp != 0)
        return fwrite(buffer, size, count, m_fp);
    return 0;
}

// The current file position. (BMPLoad, BMPSave)
long PackFile::Tell()
{
    return ftell(m_fp);
}

// Moves the file position; origin is SEEK_SET, SEEK_CUR or SEEK_END. (BMPLoad, BMPSave)
int PackFile::Seek(long offset, int origin)
{
    return fseek(m_fp, offset, origin);
}
