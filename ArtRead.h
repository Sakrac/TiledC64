#pragma once
bool LoadTileSetImages(TileMap& map, const char* mapFilename);
bool LoadPalette(const char *paletteFilename);
int GetClosestColor(uint8_t color, uint8_t* colors, int numColors);
