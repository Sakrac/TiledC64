#pragma once
bool WriteData(strref path, strref file, strref ext, void *data, size_t size);
bool WriteDataBits(strref path, strref file, strref ext, void *data, size_t size, size_t bits);
bool MergeMaps(const char *sourceListFile, const char *targetFile);
