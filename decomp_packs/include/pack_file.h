// pack_file.h : the FILE * wrapper Packs.dll's BMPLoad and BMPSave open their files with.
#ifndef PACK_FILE_H
#define PACK_FILE_H

#include <stdio.h>

class PackFile {
public:
    FILE *m_fp;  // the open stdio stream, 0 when closed

    PackFile();
    ~PackFile();
    long Tell();
    int Seek(long offset, int origin);
};

#endif
