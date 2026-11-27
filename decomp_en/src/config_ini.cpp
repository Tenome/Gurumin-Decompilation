// EN .ini helpers (JP twin: config_ini.cpp).

#include <windows.h>
#include <stdlib.h>
#include "config_ini.h"

// Value GetPrivateProfileInt returns for a missing key.
const int CONFIG_MISSING = -1;

// 1 when `key` of `section` in `file` holds an integer.
extern "C" int Config_HasInt(const char *file, const char *section, const char *key)
{
    return GetPrivateProfileIntA(section, key, CONFIG_MISSING, file) != CONFIG_MISSING;
}

// Writes integer `value` as `key` in `section` of `file`.
extern "C" int Config_SetInt(const char *file, const char *section, const char *key, int value)
{
    char text[64];
    return WritePrivateProfileStringA(section, key, _itoa(value, text, 10), file);
}

// Reads the integer `key` of `section` in `file` into *value, 0 when the key is missing. Always returns 1. Called by
// the launcher when it loads its settings.
extern "C" int Config_GetInt(const char *file, const char *section, const char *key, int *value)
{
    *value = GetPrivateProfileIntA(section, key, 0, file);
    return 1;
}

// Reads the text `key` of `section` in `file` into `buffer` (CONFIG_TEXT_BYTES bytes); the buffer is empty when the key
// is missing. Called by the launcher when it loads its settings.
extern "C" void Config_GetString(const char *file, const char *section, const char *key, char *buffer)
{
    *buffer = 0;
    GetPrivateProfileStringA(section, key, "", buffer, CONFIG_TEXT_BYTES, file);
}

// Writes the text `value` as `key` in `section` of `file`. Called by the launcher when it saves its settings.
extern "C" void Config_SetString(const char *file, const char *section, const char *key, const char *value)
{
    WritePrivateProfileStringA(section, key, value, file);
}
