#pragma once
#include "MapRead.h"
bool ConvertECBMLayer(TileMap& map, int layerIndex);
bool ConvertTextLayer(TileMap& map, int layerIndex);
bool ConvertTextMCLayer(TileMap& map, int layerIndex);
bool ConvertBitmapLayer(TileMap& map, int layerIndex);
void ConvertDataLayer(TileMap& map, int layerIndex);
bool MergeLayerData(TileMap &map, TileLayer &srcLayer, TileLayer &dstLayer);
bool MergeFlipData(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer);
bool MergeChars(TileMap& map, TileLayer& srcLayer, TileLayer& dstLayer);

// check against various flips and rotations of existing chars
uint64_t FlipChar(uint64_t o, bool flipX, bool flipY, bool rot);
int FindMatchChar(uint32_t* permutation, uint64_t charBits, const int numChars, const uint64_t* chars, bool canFlipX, bool canFlipY, bool canRot, bool canInv);


