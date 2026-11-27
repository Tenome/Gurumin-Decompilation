// file_name_utils.cpp : file-name helpers (0x10001000) and the two name-keyed checksum functions (0x10002d60,
// WaveStream::CheckKey 0x10002e70) of sound3d.dll.
#include <string.h>
#include <stdlib.h>
#include "wave_stream.h"

const char PATH_SEPARATOR = 0x5c;  // backslash

// Reduces a path to its base name: copies the path, finds the last '\' and the last '.' (a dot at position 0
// does not count), cuts the extension and copies what follows the last '\' into baseName (cleared first, 8 bytes).
// Called by the stream loaders 0x10004590 and 0x10006250 before the name checks below.
void GetBaseFileName(char *baseName, const char *path)
{
    char buffer[PATH_COPY_SIZE];
    int nameStart;
    int dotPos;
    int i;
    int j;
    char c;

    j = 0;
    do {
        c = path[j];
        buffer[j] = c;
        j++;
    } while (c != '\0');
    nameStart = 0;
    i = 0;
    dotPos = 0;
    memset(baseName, 0, BASE_NAME_SIZE);
    if (buffer[0] != '\0') {
        do {
            if (buffer[i] == PATH_SEPARATOR) {
                nameStart = i + 1;
            }
            if (buffer[i] == '.') {
                dotPos = i;
            }
        } while (buffer[++i] != '\0');
    }
    if (dotPos == 0) {
        dotPos = (int)strlen(buffer);
    }
    buffer[dotPos] = '\0';
    j = 0;
    do {
        c = buffer[nameStart + j];
        baseName[j] = c;
        j++;
    } while (c != '\0');
}

// Seed of the running XOR of both checks.
const unsigned int NAME_CHECK_XOR_SEED_A = 0xde2f;
const unsigned int NAME_CHECK_XOR_SEED_B = 0x37;
// Length of the key stream the name check cycles through (the 15 characters of the second key).
const int NAME_KEY_PERIOD = 15;
// Mask that cycles the first key (64 entries).
const int NAME_TABLE_MASK = 0x3f;

// Decodes a data block in place with a key built from the file's base name and compares two checksums with the
// expected ones: the sum and the XOR of the decoded bytes. The name (upper cased, first 8 characters) is spliced into
// the 64 byte key table at offset 16. Returns 1 when both checksums match, else 0. Called by the stream loader
// 0x10006250.
int CheckNameKeyA(char *data, int length, char *name, int expectedSum, unsigned int expectedXor)
{
    char table[70] = "!~_Q#xDRgd-d&yfg&'(8)(5(594er2f4asf5f6e5f4s5fvwyjk%fgqTUCFD4568r";
    char key[16] = "GURUguruMinmIN_";
    int sum;
    unsigned int xorSum;
    int i;
    unsigned char decoded;

    sum = 0;
    xorSum = NAME_CHECK_XOR_SEED_A;
    _strupr(name);
    strncpy(table + 16, name, BASE_NAME_SIZE);
    for (i = 0; i < length; i++) {
        decoded = (unsigned char)((key[i % NAME_KEY_PERIOD] + (i & 7)) * table[i & NAME_TABLE_MASK])
            ^ (unsigned char)~data[i];
        data[i] = decoded;
        sum += decoded;
        xorSum ^= decoded;
    }
    if (expectedSum == sum && expectedXor == xorSum) {
        return 1;
    }
    return 0;
}

// Builds a 32 byte key from the (upper cased) 8 character name, each byte the XOR of one name character with a
// constant, then sums the key's bytes (signed) and XORs them, and compares both with the expected values. Returns 1
// when they match. A member that does not use the stream (thiscall). Called by OpenEncoded (0x10004590).
int WaveStream::CheckKey(unsigned char *name, int expectedSum, unsigned int expectedXor)
{
    unsigned char key[32];
    int sum;
    unsigned int xorSum;
    int i;

    _strupr((char *)name);
    memset(key, 0, sizeof(key));
    key[1] = name[4] ^ 0xd;
    key[0] = name[6] ^ 0xf;
    key[2] = name[5] ^ 0x10;
    key[4] = name[5] ^ 0x11;
    key[3] = name[1] ^ 0x42;
    key[6] = name[7] ^ 0x3f;
    key[5] = name[3] ^ 0x2a;
    key[8] = name[3] ^ 0x23;
    key[7] = name[2] ^ 0x25;
    key[9] = name[1] ^ 0x49;
    key[10] = name[0] ^ 0x2b;
    key[14] = name[7] ^ 0x2b;
    key[15] = name[2] ^ 3;
    key[11] = name[1] ^ 0x57;
    key[12] = name[1] ^ 0x4d;
    key[19] = name[2] ^ 0x53;
    key[17] = key[6];
    key[18] = name[4] ^ 0xc;
    key[13] = name[4] ^ 0x11;
    key[25] = name[1] ^ 0x3e;
    key[16] = name[6] ^ 0x3f;
    key[24] = name[5] ^ 0x18;
    key[20] = name[5] ^ 0x35;
    key[28] = name[3] ^ 1;
    key[31] = name[4] ^ 0x17;
    key[21] = name[6] ^ 0x2c;
    key[22] = 0xe;
    key[27] = 0x49;
    key[29] = 0x2c;
    key[23] = name[3] ^ 0x49;
    key[30] = name[0] ^ 0x71;
    sum = 0;
    xorSum = NAME_CHECK_XOR_SEED_B;
    for (i = 0; i < 32; i += 8) {
        sum += (char)key[i + 1] + (char)key[i] + (char)key[i + 2] + (char)key[i + 3] + (char)key[i + 4]
            + (char)key[i + 5] + (char)key[i + 7] + (char)key[i + 6];
        xorSum ^= (char)(key[i] ^ key[i + 1] ^ key[i + 2] ^ key[i + 3] ^ key[i + 4] ^ key[i + 5] ^ key[i + 7]
            ^ key[i + 6]);
    }
    if (expectedSum == sum && expectedXor == xorSum) {
        return 1;
    }
    return 0;
}
