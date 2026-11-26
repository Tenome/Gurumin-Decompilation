// Scrambling of .itm data files (docs/ITM_FORMAT.md, tools/itm_tool.py).
// The key mixes the 15-character string "GURUguruMinmIN_", a 64-byte table and the upper-cased file name (written
// into the table at offset 8). A byte sum and an xor (seeded 0xde2f) over the plain bytes check the result.

#include <string.h>

// ItmScramble: scrambles `size` bytes in place and returns their sum and xor (taken before scrambling).
extern "C" int FUN_101614c0(unsigned char *data, int size, char *name, int *sum, unsigned int *check)
{
    int total = 0;
    unsigned int mix = 0xde2f;
    char table[70] = "!~_Q#xDRgd-d&yfg&'(8)(5(594er2f4asf5f6e5f4s5fvwyjk%fgqTUCFD4568r";
    char key[16] = "GURUguruMinmIN_";
    _strupr(name);
    strncpy(table + 8, name, 8);
    for (int i = 0; i < size; i++)
    {
        unsigned char c = data[i];
        total += c;
        mix ^= c;
        data[i] = ~((key[i % 15] + (i & 7)) * table[i & 0x3f] ^ c);
    }
    *sum = total;
    *check = mix;
    return 1;
}

// ItmDescramble: reverses ItmScramble in place; returns 1 when the sum and xor of the result match.
extern "C" int FUN_101615d0(unsigned char *data, int size, char *name, int sum, unsigned int check)
{
    int total = 0;
    unsigned int mix = 0xde2f;
    char table[70] = "!~_Q#xDRgd-d&yfg&'(8)(5(594er2f4asf5f6e5f4s5fvwyjk%fgqTUCFD4568r";
    char key[16] = "GURUguruMinmIN_";
    _strupr(name);
    strncpy(table + 8, name, 8);
    for (int i = 0; i < size; i++)
    {
        unsigned char c = (key[i % 15] + (i & 7)) * table[i & 0x3f] ^ ~data[i];
        data[i] = c;
        total += c;
        mix ^= c;
    }
    if (sum == total && check == mix)
        return 1;
    return 0;
}
