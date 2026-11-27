// file_name_utils.h : small file-name helpers and the data-file name check of sound3d.dll (the second check is
// WaveStream::CheckKey).
#ifndef FILE_NAME_UTILS_H
#define FILE_NAME_UTILS_H

// Size of the local copy of a path made by GetBaseFileName.
const int PATH_COPY_SIZE = 256;
// Longest base name kept (the output buffer is two dwords, zero filled).
const int BASE_NAME_SIZE = 8;

void GetBaseFileName(char *baseName, const char *path);    // 0x10001000
int CheckNameKeyA(char *data, int length, char *name, int expectedSum, unsigned int expectedXor);   // 0x10002d60

#endif
