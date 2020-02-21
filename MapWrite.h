#pragma once
bool WriteMapStats(TileMap& map, const char* mapFilename);
bool WriteMapData(TileMap& map, const char* mapFilename);

uint8_t* PackBits(uint8_t* orig, size_t size, size_t bits, size_t *outSize);
uint8_t* UnpackBits(uint8_t* packed, size_t count, size_t bits);
