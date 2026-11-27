// mmio_open.cpp : MmioFile::Open, the file opener of sound3d.dll (0x10003640).
//
// Reads from a plain file or from an archive: a name "dir\file.ext" is first looked up in the pack "dir.arc"
// (or "dir.dat"), whose layout is: 4 unused bytes, an extension count and then per extension a (key, offset,
// count) record that points to a table of (8 character name, size, offset) records. The file is read from the
// archive when the entry is found, otherwise from disk.
#include "mmio_file.h"
#include <string.h>

// Turns an upper case letter into lower case in place (the archives store lower case names).
#define LOWER_CASE_IN_PLACE(letter)     if ((letter) >= 'A' && (letter) <= 'Z') {         (letter) += 'a' - 'A';     }

// Opens the file. mode is the fopen-like mode string ("rb" from every caller). Returns non-zero when the file
// (or the archive entry) was opened. Called by the stream openers 0x10003ab0, 0x10003cb0, 0x10004390,
// 0x10004590, 0x10004750, 0x10004860 and 0x10006250.
int MmioFile::Open(const char *path, const char *mode)
{
    char plainPath[MMIO_PATH_BYTES];
    char directory[MMIO_PATH_BYTES];
    ArchiveName wanted;
    ArchiveName entry;
    int entrySize;
    DWORD wantedKey;
    DWORD tableCount;
    DWORD tableKey;
    DWORD tableOffset;
    DWORD entryCount;
    int nameStart;
    int nameLength;
    int i;
    DWORD entryIndex;

    if (handle != 0) {
        return 0;
    }
    if (mode[0] == 'w' || mode[0] == 'W' || mode[1] == 'w' || mode[1] == 'W') {
        handle = mmioOpen((LPSTR)path, NULL, MMIO_WRITE);
        if (handle == 0) {
            handle = mmioOpen((LPSTR)path, NULL, MMIO_CREATE | MMIO_WRITE);
        }
    } else {
        if (mode[0] != 'a' && mode[0] != 'A' && mode[1] != 'a' && mode[1] != 'A') {
            strcpy(directory, path);
            nameStart = strlen(directory);
            while (nameStart > 0 && directory[nameStart] != '\\') {
                nameStart--;
                if (directory[nameStart] == '.') {
                    directory[nameStart] = ' ';
                }
            }
            directory[nameStart] = '\0';
            char arcPath[MMIO_PATH_BYTES];
            char datPath[MMIO_PATH_BYTES];
            ArchiveKey lookup = { {{0, 0, 0, 0}}, {{0, 0, 0, 0, 0, 0, 0, 0}} };
            sprintf(datPath, "%s.dat", directory);
            sscanf(directory + nameStart + 1, "%s %s", lookup.name.text, lookup.extension.text);
            sprintf(arcPath, "%s.arc", directory);
            nameLength = strlen(lookup.name.text);
            if (nameLength > ARCHIVE_NAME_CHARS) {
                nameLength = ARCHIVE_NAME_CHARS;
            }
            for (i = 0; i < nameLength; i++) {
                LOWER_CASE_IN_PLACE(lookup.name.text[i]);
            }
            for (i = 0; i < ARCHIVE_EXTENSION_CHARS; i++) {
                LOWER_CASE_IN_PLACE(lookup.extension.text[i]);
            }
            wanted = lookup.name;
            wantedKey = lookup.extension.key;
            handle = mmioOpen(arcPath, NULL, MMIO_PARSE);
            handle = mmioOpen(arcPath, NULL, MMIO_READ);
            if (handle == 0) {
                handle = mmioOpen(datPath, NULL, MMIO_PARSE);
                handle = mmioOpen(datPath, NULL, MMIO_READ);
                if (handle == 0) {
                    strcpy(plainPath, path);
                    handle = mmioOpen(plainPath, NULL, MMIO_PARSE);
                    handle = mmioOpen(plainPath, NULL, MMIO_READ);
                    if (handle == 0) {
                        return 0;
                    }
                    isWholeFile = FILE_IS_WHOLE;
                    mmioSeek(handle, 0, SEEK_END);
                    size = Tell();
                    mmioSeek(handle, 0, SEEK_SET);
                    return 1;
                }
            }
            isWholeFile = 0;
            mmioRead(handle, (HPSTR)&tableKey, 4);
            mmioRead(handle, (HPSTR)&tableCount, 4);
            for (i = 0; i < (int)tableCount; i++) {
                mmioRead(handle, (HPSTR)&tableKey, 4);
                mmioRead(handle, (HPSTR)&tableOffset, 4);
                mmioRead(handle, (HPSTR)&entryCount, 4);
                if (tableKey == wantedKey) {
                    break;
                }
            }
            if (i == (int)tableCount) {
                mmioClose(handle, 0);
                handle = 0;
                return 0;
            }
            mmioSeek(handle, tableOffset, SEEK_SET);
            for (entryIndex = 0; entryIndex < entryCount; entryIndex++) {
                mmioRead(handle, (HPSTR)entry.word, 8);
                mmioRead(handle, (HPSTR)&entrySize, 4);
                mmioRead(handle, (HPSTR)&tableOffset, 4);
                if (entry.word[0] == wanted.word[0] && entry.word[1] == wanted.word[1]) {
                    break;
                }
            }
            if (entryIndex == entryCount) {
                mmioClose(handle, 0);
                handle = 0;
                return 0;
            }
            baseOffset = tableOffset;
            size = entrySize;
            mmioSeek(handle, tableOffset, SEEK_SET);
            return 1;
        }
        handle = mmioOpen((LPSTR)path, NULL, MMIO_READWRITE);
        mmioSeek(handle, 0, SEEK_END);
    }
    isWholeFile = FILE_IS_WHOLE;
    return handle != 0;
}
