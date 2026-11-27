// Integer settings in .ini files (EN build). Declared once here.
#ifndef GURUMIN_EN_CONFIG_INI_H
#define GURUMIN_EN_CONFIG_INI_H

// 1 when `key` of `section` in `file` holds an integer (the value reads back as something other than -1).
// EN 0x7af070 (JP 0x1015fe70 stores the value instead).
extern "C" int Config_HasInt(const char *file, const char *section, const char *key);

// Writes integer `value` as `key` in `section` of `file`; returns the Windows result. EN 0x7af370 (JP 0x1015fe20).
extern "C" int Config_SetInt(const char *file, const char *section, const char *key, int value);

// Reads integer `key` of `section` in `file` into *value (0 when missing). Returns 1. EN 0x7af230.
extern "C" int Config_GetInt(const char *file, const char *section, const char *key, int *value);

// Reads text `key` of `section` in `file` into `buffer` (CONFIG_TEXT_BYTES bytes; empty when missing). EN 0x7af250.
extern "C" void Config_GetString(const char *file, const char *section, const char *key, char *buffer);

// Writes text `value` as `key` in `section` of `file`. EN 0x7af3b0.
extern "C" void Config_SetString(const char *file, const char *section, const char *key, const char *value);

// Size of the text buffer Config_GetString fills.
const int CONFIG_TEXT_BYTES = 0x80;

#endif
