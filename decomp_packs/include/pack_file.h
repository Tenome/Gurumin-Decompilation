// pack_file.h : the FILE * wrapper Packs.dll's BMPLoad and BMPSave open their files with.
#ifndef PACK_FILE_H
#define PACK_FILE_H

#include <stdio.h>

// Size of the base-name buffer kept next to the stream (file name without directory and extension).
const int PACK_FILE_NAME_SIZE = 0x24;

// Largest block handed to fread in one call; bigger reads are split into blocks of this size.
const int PACK_FILE_READ_BLOCK = 0x8000;

// Keeps the part of a path between the last backslash and the last dot. Returns 0.
int ExtractBaseName(char *baseName, const char *path);

class PackFile {
public:
    FILE *m_fp;                         // the open stdio stream, 0 when closed
    char m_name[PACK_FILE_NAME_SIZE];   // base name of the opened file (no directory, no extension)

    PackFile();
    ~PackFile();
    void Close();
    int Open(const char *path, const char *mode);
    size_t Read(void *buffer, size_t size, size_t count);
    size_t Write(const void *buffer, size_t size, size_t count);
    long Tell();
    int Seek(long offset, int origin);
};

#endif
