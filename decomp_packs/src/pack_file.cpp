// pack_file.cpp : PackFile, the stdio wrapper BMPLoad and BMPSave use for their image files.
#include "pack_file.h"

// Starts closed. (BMPLoad, BMPSave)
PackFile::PackFile()
{
    m_fp = 0;
}

// Closes the stream if it is still open. (BMPLoad, BMPSave, their unwind funclets)
PackFile::~PackFile()
{
    if (m_fp != 0)
        fclose(m_fp);
    m_fp = 0;
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
